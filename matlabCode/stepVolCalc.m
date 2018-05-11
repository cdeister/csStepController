%% Basic Info

% 1/16th microstepping is effectivley the limit to conserve torque for most
% situations. 
%
% http://reprap.org/wiki/Step_rates
% https://www.micromo.com/technical-library/stepper-motor-tutorials/microstepping-myths-and-realities
% https://duet3d.dozuki.com/Wiki/Choosing_and_connecting_stepper_motors
% (amazing)
% https://hackaday.com/2016/08/29/how-accurate-is-microstepping-really/
% http://users.ece.utexas.edu/~valvano/Datasheets/StepperMicrostep.pdf

% a) get step rate
% http://reprap.org/wiki/Triffid_Hunter%27s_Calibration_Guide#XY_steps
stepAngle=1.8; % 1.8 (200 steps) is standard. If you paid more than $30 for your motor, double check.
microSteps=256;  % 16 is most common, but can micro-higher. 
pitchInMM=1.25;

stepsPerRev=360/stepAngle;
stepsPerMM=(stepsPerRev)/pitchInMM;
muStepsPerMM=(stepsPerRev*microSteps)/pitchInMM;


% b) get length of syringe
% http://www.harvardapparatus.com/media/harvard/pdf/Syringe%20Selection%20Guide.pdf
%http://www.restek.com/norm-ject-specs
% http://www.cchem.berkeley.edu/rsgrp/Syringediameters.pdf
syringeVolumeInML=30;
syringeDiamInMM=22.7;

syringeLengthInMM=(syringeVolumeInML)/(pi*((syringeDiamInMM/2)^2))*1000;

% c) vol per mm
volPerMM=(syringeVolumeInML/syringeLengthInMM)
% 0.4047 mL for a 2mm rod
volPerStep=volPerMM/stepsPerMM;
% 0.004 mL/step with no microstepping 


volPerMicrostep=volPerStep/microSteps;
% 0.00025 mL or 0.25 ul per microstep (1/16th stepping).

%125 psi is 86.18 N/cm2
% stepper motor/speed
% http://www.nmbtc.com/step-motors/engineering/torque-and-speed-relationship/
% http://www.orientalmotor.com/stepper-motors/technology/speed-torque-curves-for-stepper-motors.html
% https://www.linengineering.com/news/in-the-media/how-to-use-microstepping-to-get-more-torque/

% c) factor in acceleration
stepperRate=1; % mm/sec
stepperTime=0.01; % in seconds
volPerTimeInML=(stepperRate*stepperTime)*volPerMM
stepsPerReward=volPerTimeInML/volPerStep
microStepsPerReward=volPerTimeInML/volPerMicrostep

% ----------- will document rest of torque later.
% d) torque considerations https://www.micromo.com/microstepping-myths-and-realities
holdTorque=0.310870805; % varies by motor mine is 3.17 kg*cm
% incTorque=holdTorque*sin();


%% connect to the controller with the matrix.
comPath = '/dev/cu.usbmodem1451'; %COMX on windows. Change this maybe.
stepper=serial(comPath,'BaudRate',9600);
fopen(stepper);


%% check variable examples
% get current variables  {'m', 't', 'd', 's', 'a', 'z'};
%  m=microSteps; t=totalSteps; d='direction'; s=speed; a=acel; z=state;

% to check a variable, use the code above and end with a '<'
% this will return a string that starts with 'echo' and proceded by the
% variable char code, then its value. 

% here is how I parse what the controller thinks the microstep setting (m) is
fprintf(stepper,'m<');
tempBuf=fscanf(stepper);
splitBuf=strsplit(tempBuf,',');
if strcmp(splitBuf{1},'echo') && strcmp(splitBuf{2},'m')
    microSteps=str2num(splitBuf{3});
end

% we can write a function for this
% function parsedVar=checkSerVar(comObj,tChar)
%     fprintf(comObj,[tChar '<']);
%     tempBuf=fscanf(comObj);
%     splitBuf=strsplit(tempBuf,',');
%     if strcmp(splitBuf{1},'echo') && strcmp(splitBuf{2},tChar)
%         parsedVar=str2num(splitBuf{3});
%     else 
%         parsedVar=[];
%     end
% end
% %     

% add the matlab code folder to your path now and try
microSteps=checkSerVar(stepper,'m');

%% now loop
availableVars={'m', 't', 'd', 's', 'a', 'z'};
availableVarsNames={'sMicroSteps', 'sTotalSteps', 'sDirection', 'sSpeed', 'sAcel', 'sState'};
for n=1:numel(availableVarsNames)
    eval(['stepperVars.' availableVarsNames{n} '= checkSerVar(stepper,availableVars{n});'])
end

%% now look at the reward command (just the aceleration). 
% I will document decel later.
% always keep in mind 809 ul per turn
% at 200 steps that is 4.05 ul per full step
% i recomend no less than a 1/16th step that gives 0.25 ul per 1/16th step

% make acceleration plot
% v(t)=int(a(t)dt)
% s = 	s0 + v0t + ½at2

tMicro=64;
totalSteps=20;
volPerStep=0.004;
totalSteps=(totalSteps/tMicro)*tMicro;
sTimes=zeros(totalSteps,1);
sMap=zeros(totalSteps,1);
pDT=0.01;
rewardTime=0.5;
firstPulsePred=rewardTime/32.455
rTime=0:pDT:1.5;
sTimes(1)=firstPulsePred;
sMap(1)=1;

for n=2:totalSteps
    sTimes(n)=sTimes(n-1)-((2*sTimes(n-1))/(4*n+1));
    sMap(n)=sMap(n-1)+1;
    
    
end

% figure,plot(cumsum(sTimes),sMap)
% ylabel('steps')
% xlabel('time')
% % stepperVars.sAcel



% Now we will plot volume

volPerMicrostep=(volPerStep/tMicro)*1000;
figure,plot(cumsum(sTimes),sMap*volPerMicrostep,'b-','linewidth',1)
hold all,plot([0 rewardTime],[totalSteps*volPerMicrostep totalSteps*volPerMicrostep],'k:','linewidth',1)
ylabel('volume(ul)')
xlabel('time')








