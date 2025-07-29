# Singularity-free Twist Limit Constraints for the Ball Joint

## Instructions to Run
1. Open with Visual Studio 2022, the code was tested with Windows SDK version 10.0.22000.0.
2. Go to Solution Explorer, right click `BallJointSim`, choose `Set as Startup Project`.
3. Then select `Properties` for `BallJointSim`, choose `Debugging`, the commmand arguments are explained down below.
- **-console**
  - `0`: disable console terminal window  
  - `1`: enable console terminal window 
  - Generally `1` is used.

- **-rm** (render mode)
  - `0`: rendering thread waits for logic thread to submit new data  
  - `1`: rendering thread grabs whatever is in the buffer submitted by logic thread  
  - `2`: no rendering
  - Generally `1` is used.

- **-example**
  - `1` to `12`: See function `RunUnitTest()` from  
    `Simulation_\BallJointSim\MultiBodyUnitTest.cpp` for all test cases
  - `1`: section 5.1 in the paper
  - `2`: section 5.2 in the paper
  - `3`: section 5.3 in the paper
  - `4`: section 5.3 in the paper
  - `5`: section 5.4 in the paper
  - `6`: section 5.4 in the paper
  - `7`: section 5.5 in the paper
  - `8`: section 5.6 in the paper
  - `9`: section 5.7 in the paper
  - `10`: section 5.8 in the paper
  - `11`: section 5.8 in the paper
  - `12`: the example used in the presentation
  

- **-tm** (twisting constraint mode)
  - `0`: direct swing twist 
  - `1`: alterative Euler twist
  - `2`: incremental  
  - `3`: Euler twist 

- **-ps** (position solve)
  - `0`: disable position solve  
  - `1`: enable position solve
  
- **-damping**
  - `(0, 1]`: `1` means no damping, any value between `0` and `1` results a velocity damping

![](Images/arguments.png)

5. For build configurations, choose either `Debug` or `Release`, choose `x64` platform. Now you can compile and run.
6. (optional) Copy `setting.ini` under `TwistLimitSim\Simulation_` to `TwistLimitSim\temp\x64\Debug(Release)\Simulation_`, you can change the rendering resolution by modfiying `setting.ini`.
Note, folder `temp` will only exist after the soltuion was build first.

## Control Instructions
1. Hold the right mouse button to rotate camera.
2. Press `W`, `S`, `A`, `D` to move forward, backward, left and right when holding the right mouse button.
3. Press `Space` bar to start simulation.
4. Press `G` to simluate a single time step.

## Code Overview
1. Twist limit is implmented under `TwistLimitSim\Simulation_\BallJointSim\JointLimit.cpp`.
2. Equations of motions for the ball joint in generalized cooridiantes is implmented under `TwistLimitSim\Simulation_\BallJointSim\MultiBody.cpp`.
3. Simulation dt can be changed in `TwistLimitSim\Engine\Application\cbApplication.h` by searching function `virtual double GetSimulationUpdatePeriod_inSeconds()`, the default value is 1/1000.
    