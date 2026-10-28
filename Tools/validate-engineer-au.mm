#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>
#include <CoreFoundation/CoreFoundation.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

// Direct bundle/factory host for CI machines that do not discover a fresh third-party AU installation.
// This does not bypass AU initialization, parameter/state or rendering checks.
static float inputData[2][4096]{};
static OSStatus input(void*,AudioUnitRenderActionFlags*,const AudioTimeStamp* time,
                      UInt32,UInt32 frames,AudioBufferList* audio){
  for(UInt32 c=0;c<audio->mNumberBuffers;++c){auto* p=static_cast<float*>(audio->mBuffers[c].mData);if(frames>4096)return -50;if(!p){p=inputData[c%2];audio->mBuffers[c].mData=p;audio->mBuffers[c].mDataByteSize=frames*sizeof(float);}for(UInt32 i=0;i<frames;++i)p[i]=.15f*std::sin(float(6.28318530718*223*(time->mSampleTime+i)/48000));}
  return noErr;
}
static bool ok(OSStatus value,const char* what){if(value!=noErr)std::fprintf(stderr,"FAIL: %s OSStatus=%d\n",what,int(value));return value==noErr;}
int main(int argc,char** argv){
  if(argc!=2)return 1;
  auto url=CFURLCreateFromFileSystemRepresentation(nullptr,reinterpret_cast<const UInt8*>(argv[1]),CFIndex(std::strlen(argv[1])),true);
  auto bundle=CFBundleCreate(nullptr,url);CFRelease(url);if(!bundle){std::puts("FAIL: CFBundleCreate");return 2;}
  CFErrorRef error=nullptr;
  if(!CFBundleLoadExecutableAndReturnError(bundle,&error)){if(error){CFShow(error);CFRelease(error);}std::puts("FAIL: cannot load AU executable");return 3;}
  auto factory=reinterpret_cast<AudioComponentFactoryFunction>(CFBundleGetFunctionPointerForName(bundle,CFSTR("Makkine_Engineer_5X___AI_Vocal_Tuner_EvolutionAUFactory")));
  if(!factory){std::puts("FAIL: missing exported AU factory");return 4;}
  AudioComponentDescription description{'aufx','E5x1','M7G7',0,0};
  auto component=AudioComponentRegister(&description,CFSTR("M7G: Makkine Engineer 5X"),0x10000,factory);
  if(!component){std::puts("FAIL: direct AU factory registration");return 5;}
  AudioUnit unit=nullptr;if(!ok(AudioComponentInstanceNew(component,&unit),"instantiate"))return 6;
  AudioStreamBasicDescription format{};format.mSampleRate=48000;format.mFormatID=kAudioFormatLinearPCM;format.mFormatFlags=kAudioFormatFlagIsFloat|kAudioFormatFlagIsPacked|kAudioFormatFlagIsNonInterleaved;format.mBytesPerPacket=format.mBytesPerFrame=4;format.mFramesPerPacket=1;format.mChannelsPerFrame=2;format.mBitsPerChannel=32;
  if(!ok(AudioUnitSetProperty(unit,kAudioUnitProperty_StreamFormat,kAudioUnitScope_Input,0,&format,sizeof(format)),"input format")||!ok(AudioUnitSetProperty(unit,kAudioUnitProperty_StreamFormat,kAudioUnitScope_Output,0,&format,sizeof(format)),"output format"))return 7;
  AURenderCallbackStruct callback{input,nullptr};if(!ok(AudioUnitSetProperty(unit,kAudioUnitProperty_SetRenderCallback,kAudioUnitScope_Input,0,&callback,sizeof(callback)),"input callback"))return 8;
  UInt32 maximum=256;AudioUnitSetProperty(unit,kAudioUnitProperty_MaximumFramesPerSlice,kAudioUnitScope_Global,0,&maximum,sizeof(maximum));
  if(!ok(AudioUnitInitialize(unit),"initialize"))return 9;
  UInt32 parameterBytes=0;Boolean writable=false;if(!ok(AudioUnitGetPropertyInfo(unit,kAudioUnitProperty_ParameterList,kAudioUnitScope_Global,0,&parameterBytes,&writable),"parameter list"))return 10;
  if(parameterBytes<4*sizeof(AudioUnitParameterID)){std::puts("FAIL: missing tuner parameters");return 11;}
  struct Buffers {UInt32 count;AudioBuffer buffers[2];} output{};output.count=2;float data[2][256]{};
  for(int c=0;c<2;++c)output.buffers[c]={1,sizeof(data[c]),data[c]};
  double energy=0;
  for(int block=0;block<200;++block){AudioTimeStamp time{};time.mSampleTime=block*256.;time.mFlags=kAudioTimeStampSampleTimeValid;AudioUnitRenderActionFlags flags=0;if(!ok(AudioUnitRender(unit,&flags,&time,0,256,reinterpret_cast<AudioBufferList*>(&output)),"render"))return 12;for(auto& channel:data)for(float sample:channel){if(!std::isfinite(sample)||std::abs(sample)>1.001f)return 13;energy+=sample*sample;}}
  if(energy<1){std::puts("FAIL: AU did not process actual input");return 14;}
  CFPropertyListRef state=nullptr;UInt32 stateBytes=sizeof(state);
  if(!ok(AudioUnitGetProperty(unit,kAudioUnitProperty_ClassInfo,kAudioUnitScope_Global,0,&state,&stateBytes),"get state")||!state)return 15;
  if(!ok(AudioUnitSetProperty(unit,kAudioUnitProperty_ClassInfo,kAudioUnitScope_Global,0,&state,sizeof(state)),"restore state"))return 16;CFRelease(state);
  AudioUnitUninitialize(unit);AudioComponentInstanceDispose(unit); // Keep the registered factory bundle loaded until process exit.
  std::printf("PASS: native AU bundle load, exported factory, initialization, stereo input/output, %u parameters, state roundtrip and actual rendering; energy=%.3f\n",parameterBytes/unsigned(sizeof(AudioUnitParameterID)),energy);
  return 0;
}
