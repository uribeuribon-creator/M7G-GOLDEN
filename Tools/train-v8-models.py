"""Reproducible synthetic-only training; no recordings or proprietary models."""
from pathlib import Path
import json
import numpy as np
from sklearn.neural_network import MLPRegressor
from sklearn.metrics import mean_absolute_error, balanced_accuracy_score
ROOT=Path(__file__).resolve().parents[1]
rng=np.random.default_rng(87123)
# Auto-mix distils a documented bounded engineering policy, not musical taste.
x=rng.uniform(0,1,(18000,12)).astype(np.float32)
y=np.column_stack((.65-.35*x[:,1]+.12*x[:,3], .65-.45*x[:,2]-.18*x[:,8],
 .35+.4*x[:,1], .25+.65*x[:,2], .7*x[:,10]*(1-.35*x[:,9]),
 .2+.5*x[:,6]+.2*x[:,2], .25+.35*x[:,9]-.15*x[:,10], .15+.65*x[:,9]))
y=np.clip(y,0,1).astype(np.float32)
mlp=MLPRegressor(hidden_layer_sizes=(16,),activation='tanh',max_iter=180,
 random_state=8100,batch_size=256,early_stopping=True,tol=1e-6).fit(x[:15000],y[:15000])
mae=float(mean_absolute_error(y[15000:],mlp.predict(x[15000:])))
# Candidate-scoring CNN. Inputs: ACF and YIN CMND at relative candidate lags.
def data(seed, count):
 r=np.random.default_rng(seed); features=[]; labels=[]
 sr=22050; n=2048; t=np.arange(n)/sr
 for k in range(count):
  f=float(np.exp(r.uniform(np.log(65),np.log(950))))
  phase=2*np.pi*f*t+ r.uniform(0,2*np.pi)
  signal=np.zeros(n)
  for h in range(1,13):
   amp=(r.uniform(.1,1)/h**r.uniform(.6,1.5))
   if h==1 and r.random()<.35:amp*=.05
   signal+=amp*np.sin(h*phase+r.uniform(-.2,.2))
  signal*=np.linspace(r.uniform(.3,1),r.uniform(.3,1),n)
  signal+=r.normal(0,np.std(signal)*r.uniform(.01,.45),n)
  if k%9==0:signal=r.normal(0,1,n)
  spectrum=np.fft.rfft(signal,4096)
  corr=np.fft.irfft(spectrum*np.conj(spectrum),4096)[:1025]
  energy=np.r_[0,np.cumsum(signal*signal)]
  lag=np.arange(1025); ex=energy[n-lag]; ey=energy[n]-energy[lag]
  acf=corr/np.sqrt(np.maximum(1e-15,ex*ey))
  diff=np.maximum(0,ex+ey-2*corr); cmnd=np.ones(1025)
  cmnd[1:]=diff[1:]*lag[1:]/np.maximum(1e-15,np.cumsum(diff[1:]))
  for multiple in (1,.5,2,r.uniform(.62,1.65)):
   candidate=sr/(f*multiple)
   at=np.clip(np.linspace(.5,2.5,32)*candidate,1,1024)
   features.append(np.array([np.interp(at,lag,acf),1-np.minimum(2,np.interp(at,lag,cmnd))]))
   labels.append(float(k%9!=0 and abs(multiple-1)<.035))
 return np.asarray(features,dtype=np.float32),np.asarray(labels,dtype=np.float32)[:,None]
X,Y=data(81001,3200); XT,YT=data(81991,900)
# Two input channels, four learned kernels, tanh, dense 12, sigmoid.
W=rng.normal(0,.12,(4,10)).astype(np.float32); b=np.zeros(4,dtype=np.float32)
D=rng.normal(0,.10,(128,12)).astype(np.float32); db=np.zeros(12,dtype=np.float32)
O=rng.normal(0,.1,(12,1)).astype(np.float32); ob=np.zeros(1,dtype=np.float32)
params=[W,b,D,db,O,ob]; mom=[np.zeros_like(v) for v in params]; vel=[np.zeros_like(v) for v in params]
def windows(a):
 p=np.pad(a,((0,0),(0,0),(2,2)))
 return np.lib.stride_tricks.sliding_window_view(p,5,axis=2).transpose(0,2,1,3).reshape(-1,32,10)
WX=windows(X); WXT=windows(XT)
def forward(a):
 c=np.tanh(a@W.T+b); flat=c.reshape(-1,128); hidden=np.tanh(flat@D+db)
 out=1/(1+np.exp(-np.clip(hidden@O+ob,-30,30)))
 return c,flat,hidden,out
step=0
for epoch in range(45):
 order=rng.permutation(len(X))
 for start in range(0,len(X),128):
  idx=order[start:start+128]; a=WX[idx]; target=Y[idx]
  c,flat,h,out=forward(a); delta=(out-target)/len(idx)
  go=h.T@delta; gob=delta.sum(0)
  dh=(delta@O.T)*(1-h*h); gd=flat.T@dh; gdb=dh.sum(0)
  dc=(dh@D.T).reshape(-1,32,4)*(1-c*c)
  gw=dc.reshape(-1,4).T@a.reshape(-1,10); gb=dc.sum((0,1))
  step+=1
  for p,m,v,g in zip(params,mom,vel,[gw,gb,gd,gdb,go,gob]):
   m*=.9;m+=.1*g;v*=.999;v+=.001*g*g
   p-=.002*(m/(1-.9**step))/(np.sqrt(v/(1-.999**step))+1e-8)
score=forward(WXT)[-1]
accuracy=float(balanced_accuracy_score(YT[:,0],score[:,0]>.5))
assert mae<.04,mae
assert accuracy>.80,accuracy
# Compact plain float arrays, architecture implemented independently in C++.
def array(name,a):
 vals=np.asarray(a,dtype=np.float32).ravel()
 return 'inline constexpr std::array<float, %d> %s{{%s}};\n'%(len(vals),name,','.join(f'{float(v):.9e}f' for v in vals))
header='#pragma once\n#include <array>\nnamespace v8models {\n'
for name,a in [('mixW1',mlp.coefs_[0]),('mixB1',mlp.intercepts_[0]),('mixW2',mlp.coefs_[1]),('mixB2',mlp.intercepts_[1]),('pitchKernels',W),('pitchBias',b),('pitchDense',D),('pitchDenseBias',db),('pitchOut',O),('pitchOutBias',ob)]:header+=array(name,a)
header+='}\n'
(ROOT/'Source/V8ModelWeights.h').write_text(header)
metrics={'training_data':'synthetic harmonic signals and a bounded DSP teacher policy; no commercial recordings',
 'auto_mix':{'architecture':'12 -> 16 tanh -> 8 linear','held_out_samples':3000,'mean_absolute_error':mae},
 'pitch_cnn':{'architecture':'2x32 -> Conv1d(4,kernel=5,pad=2) tanh -> flatten -> dense12 tanh -> sigmoid',
 'training_candidates':len(X),'held_out_candidates':len(XT),'held_out_seed':81991,'balanced_accuracy':accuracy},
 'limits':['Synthetic evaluation is not validation on real singers.','Auto-mix learns engineering targets, not professional aesthetic preferences.','CNN scores pitch candidates; it does not separate polyphonic notes.'],
 'license':'Own generated weights; RNNoise is separate and retains its original BSD license.'}
(ROOT/'Models/V8_METRICS.json').write_text(json.dumps(metrics,indent=2))
print(json.dumps(metrics,indent=2),flush=True)
