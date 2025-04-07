# cubic-solver
A demonstration of cubic easing.
Click anywhere on the screen to add a target (the green square).
The entity (blue square) eases toward the target according to a cubic function.
The cubic function is constrained such that current position and velocity is maintained, while reaching the target after one second with zero velocity.
To configure the time it takes to reach the target, as well as other constants, change the value of `TARGET_TIME`.

## Prerequisits
To build, you need a C-compiler supporting C23.
You will also need to install and link with SDL3.
### Example usage
```
$ gcc -O3 -std=c23 -o cubic_solver -lSDL3
$ ./cubic_solver
```
