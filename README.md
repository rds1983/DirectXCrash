## Overview
I was working upon a game engine and encountered weird bug: the sample crashed with the access violation after a few seconds of work.

I’ve spend more work to localize the problem and came with minimal code that reproduces the crash, which resulted in this repo.

## Bug Traits
* The bug is very "tender". Slightlest changes in the logic make it go away. I.e. if you change line 70 of CameraMotionBlur.fx to `output.FrustumRay = FrustumCorners[2];`, then the bug goes away. Similarly it goes away if the app is run in 32-bit mode.
* It reproduced on NVidia cards. But it didn't reproduce on Intel HD 4000. Could be the NVidia driver bug.

## How To Reproduce
1. Clone this repo.
2. Open the solution in the IDE, compile and run.
3. It'll crash in the period from 1 to 30 seconds:

https://github.com/rds1983/DirectXCrash/assets/1057289/cc6fcafa-94eb-49e6-a870-a86e578061a0


