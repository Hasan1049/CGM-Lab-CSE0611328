# Colour Lab

A colour mixing game for kids. It is made for the Computer Graphics
and Multimedia Lab with C++, OpenGL 3.3 core, GLFW, GLAD and GLM.

# Submitted To:

# Audity Ghosh
 Lecturer, UITS
  
# Sirajam Samira
 Lecturer, UITS


# Submitted By:
 Md Hasan Mazumder  ID: 0432410005101049
 Salimullah Pavel   ID: 0432410005101048
 Ashik Sarker       ID: 0432410005101017

## The idea
* There are 4 simple shapes: sunflower, apple, fish and balloon.
* The shape on the left is coloured with a secret mix of 3 palette colours.
* The kid presses 3 colour keys (R, B, Y, G, M, C) and the same shape in the
  middle gets the mixed colour.
* Enter checks the answer. A correct mix goes straight to the next shape, a
  wrong mix clears the colours so the same shape can be tried again.
* After all 4 shapes the game shows a winning cup. Space starts it again.

## How to run (Windows + MinGW)
1. Open the `ColourLab` folder in VS Code and open a terminal.
2. Type `make` (or `mingw32-make` if `make` is not found).
   This builds the game into `build/` and starts it.
3. `make clean` deletes the build files.

## Folder structure
```
ColourLab/
  build/          compiled files (created by make)
  include/        glad.h, glfw3.h, KHR/, glm/ (headers)
  lib/            libglfw3.a (GLFW library for MinGW 64 bit)
  src/            main.cpp (the game) and glad.c
  Makefile        build and run commands
  instructions.txt  how to play
  read.md         quick start
  ReadMe.md       this file
```

## How the colour mixing works
* Each palette colour has an RGB value.
* The mixed colour is the average of the chosen colours' RGB values.
  Example: red + yellow gives orange.
* Every shape has a secret combination of 3 colours (the `solution` array in
  `main.cpp`). The target colour is the average of those 3 colours.
* When Enter is pressed the game checks if every RGB channel of the player's
  mix is within 0.01 of the target. The order of the colours does not matter.

## Transformations used
* Rotate: the sunflower petals turn, and the cup bowl is a trapezoid turned
  upside down.
* Translate: the fish moves up and down along the y axis, and the dot of the
  current shape bounces.
* Scale: a colour button grows a little when its key is pressed.

## Change a puzzle
Open `src/main.cpp` and edit the `solution` array. The numbers are palette
colours: 0 = red, 1 = blue, 2 = yellow, 3 = green, 4 = purple, 5 = cyan.
