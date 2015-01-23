Fault Terrain
===============================
A C++ OpenGL program to produce an interactive terrain mesh.
By Don Pham and Erica Cheyne.


Required libraries
------------------
 - GLUT / Freeglut
 - Tested on Windows and Debian/Ubuntu.
 - Along with the Makefile, a Visual Studios 2013 solution file is included.
 
Additional Features
-------------------------------
 * Improved Camera - The camera has freelook (with constraints) and moves independently from where it's looking. The camera can look diagonally as well by pressing multiple keys at once.[1](https://www.opengl.org/discussion_boards/showthread.php/131581-GLUT-two-keys-at-once?p=979717&viewfull=1#post979717)
 * Topographic map coloring.
 * Mouse drags change the camera angle.
 * Circle faulting algorithm - Best seen if you reset the terrain first before using it. (the faults only grow upwards)

Controls
-------------------------------
```
 * p         : passiveFaulting : continuously do 1 fault every update
 * <space>   : do 800 faults
 * l         : toggle lighting
 * k		 : toggle shaders
 * w		 : toggle between filled polygons, wireframe, and both
 * f		 : toggle between line faulting and circle faulting algorithm
 * r		 : reset the scene and camera position
 * PgUp,PgDn : move the camera forwards and backwards (alt. keys: Home and End)
 * arrows    : rotate the camera around its own axis
 * mouse	 : rotate the camera on its y-axis only (click and drag)
 * s,x,z,c   : Move first light up,down,backwards,forwards
 * g,b,v,n   : Move second light up,down,backwards,forwards
 * < >       : Rotate the terrain left and right (alt. keys: comma and period)
 * : ?       : Rotate the terrain on a diagonal axis (alt. keys: semi-colon and forward-slash)
```

![Screenshot](screenshot.gif?raw=true)