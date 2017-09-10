# shell
A simple shell written in C++

The shell can run the built-in commands below or any commands in the /bin or /usr/bin folders.
Built-in commands have precedence over commands in these folders if names are shared. 

Built-In commands:
  exit
    Exit the shell
    
  set
    Set local variable
    Usage: set [var name] [var value]
    
  unset
    Delete local variable
    Usage unset [var name]
  
  prt
    Prints everything after prt and interpolates variables (distinguised by "$" character at beginning of word)
  
  envset
    Set environment variable
    Usage: envset [var name] [var value]
    
  envunset
    Delete environment variable
    Usage envunset [var name]
    
  envprt
    Print variables and values in environment
    Usage: envprt
  
  witch
    Works like bash "which" command
    Usage: witch [command]
  
  pwd
    Print working director
    Usage: pwd
  
  cd
    Change working directory
    Usage: cd [relative or complete path]
    
  lim
    Set or print limits
    Usage:  lim (prints limits)
            lim [cpu time limit in seconds] [membory space limit in MB] (sets cpu and mem limits)
  
  jobs
    Lists current background/suspended jobs. A job is suspended by pressing ctrl-z.
    Usage: jobs
    
  fg
    Brings background/suspended job to foreground
    Usage: fg [job id (get from jobs command)]
  
  bg
    Run suspended job in background
    Usage bg [suspended job id (get from jobs command)]
    
  kill
    Send kill signal to background/suspended process
     Usage kill [job id (get from jobs command)]
  
  shmalloc
    Allocate shared memory space
    Usage: shmalloc [name] [size in MB]
    
  shmdel
    Delete shared memory space
    Usage: shmdel [name]

