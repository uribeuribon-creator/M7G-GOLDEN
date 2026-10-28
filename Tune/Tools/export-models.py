"""Export original pitch CNN and train acoustic-profile MLP. Synthetic-only data."""
from pathlib import Path
import re,json,hashlib
import numpy as np
import onnx
from onnx import helper as H, numpy_helper as N, TensorProto as T
from sklearn.neural_network import MLPRegressor
root=Path(__file__).resolve().parents[2]; out=root/'Tune/Models'
rng=np.random.default_rng(712030)
def init(name,a):return N.from_array(np.asarray(a,dtype=np.float32),name)
def save(name,nodes,inputs,outputs,weights):
 g=H.make_graph(nodes,name,inputs,outputs,weights)
 m=H.make_model(g,opset_imports=[H.make_opsetid('',17)],producer_name='Makkine Seven Golden / original synthetic models')
 m.ir_version=9;onnx.checker.check_model(m);onnx.save(m,out/(name+'.onnx'))
s=(root/'Source/V8ModelWeights.h').read_text()
def arr(name,shape):
 m=re.search(r'\b'+name+r'\s*\{\s*\{(.*?)\}\s*\}',s,re.S)
 return np.array([float(v.strip().rstrip('f')) for v in m.group(1).split(',')],np.float32).reshape(shape)
save('AlienPitchCandidate',[
 H.make_node('Conv',['features','kernel','bias'],['c'],pads=[2,2]),H.make_node('Tanh',['c'],['t']),
 H.make_node('Transpose',['t'],['p'],perm=[0,2,1]),H.make_node('Flatten',['p'],['f'],axis=1),
 H.make_node('Gemm',['f','dense','db'],['h']),H.make_node('Tanh',['h'],['ht']),
 H.make_node('Gemm',['ht','final','fb'],['logit']),H.make_node('Sigmoid',['logit'],['probability'])],
 [H.make_tensor_value_info('features',T.FLOAT,[3,2,32])],
 [H.make_tensor_value_info('probability',T.FLOAT,[3,1])],
 [init('kernel',arr('pitchKernels',(4,2,5))),init('bias',arr('pitchBias',(4,))),
 init('dense',arr('pitchDense',(128,12))),init('db',arr('pitchDenseBias',(12,))),
 init('final',arr('pitchOut',(12,1))),init('fb',arr('pitchOutBias',(1,)))])
# Feature-policy distillation, not an identity/gender/genre recognizer.
# f0 MIDI-normalized, periodicity, air ratio, roughness, centroid, low/mid/high
# energy, zero crossing rate, crest, RMS, noise floor. Targets have physical
# feature semantics; no personal characteristics or commercial audio is used.
x=rng.uniform(0,1,(18000,12)).astype(np.float32)
y=np.column_stack((1/(1+np.exp((x[:,0]-.37)*20)),
 1/(1+np.exp((.60-x[:,0])*20)),x[:,2]**.7,(x[:,3]*(1-.35*x[:,1]))**.8)).astype(np.float32)
m=MLPRegressor(hidden_layer_sizes=(20,),activation='tanh',solver='adam',max_iter=650,
 random_state=712,batch_size=256,early_stopping=True,tol=1e-7).fit(x[:15000],y[:15000])
mae=float(np.mean(np.abs(m.predict(x[15000:])-y[15000:])))
assert mae<.035,mae
save('AlienVoiceProfile',[H.make_node('Gemm',['features','w1','b1'],['h']),H.make_node('Tanh',['h'],['t']),
 H.make_node('Gemm',['t','w2','b2'],['profile'])],
 [H.make_tensor_value_info('features',T.FLOAT,[1,12])],[H.make_tensor_value_info('profile',T.FLOAT,[1,4])],
 [init('w1',m.coefs_[0]),init('b1',m.intercepts_[0]),init('w2',m.coefs_[1]),init('b2',m.intercepts_[1])])
report={'pitch':json.loads((root/'Models/V8_METRICS.json').read_text())['pitch_cnn'],
 'profile':{'architecture':'12 -> 20 tanh -> 4 linear, clamped at runtime','held_out_samples':3000,'mean_absolute_error':mae,
 'targets':['low register','high register','air/breath feature','roughness feature'],
 'training':'Synthetic feature-policy distillation, not recorded real voices; not sex/gender/genre or anatomical-register inference.'},
 'limits':['Pitch CNN scores YIN candidates; not CREPE and not polyphonic transcription.',
 'Five tuning profiles are different DSP control curves, not five independently trained generative voice models.',
 'Harmonies/doubles use original spectral resynthesis; not generative singers or genuine additional takes.',
 'Synthetic held-out scores do not establish accuracy on real singers.'],
 'onnxruntime':'CPU 1.20.1, on a bounded worker thread; no inference on audio callback',
 'sha256':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in out.glob('*.onnx')}}
(out/'METRICS.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
