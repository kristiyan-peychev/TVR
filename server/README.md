#The server side

##Compilation
This here compilation is kind of easier than that on the client side.
You should just type `make` and it should compile.

In case I have forgotten to add a Makefile, which I _have_ right now, just executing `gcc *.c -lpthread` should be enough for the server to compile properly.

###Dependencies
As mentioned above, the server depends on *pthreads*, thus if you do not have the pthread development libraries you should get them. Though it should be included in most Linux distribution's glibc development package, if you have a packet manager _at all_. 

Yeah, you guessed it, this should compile and run successfully on all Linux distros and perhaps the BSD ones, which I will leave untested, just because.

##Execution
The Makefile will probably create a file named 'server', witch you are to simply run. The server should handle all connection issues automatically. 

If there are any issues please report them to me.
