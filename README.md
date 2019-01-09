# tarappend
An utility enabling parallel (asynchronous) writing to tar archives

This is an utility for UNIX-like systems that enables parallel writing to a .tar archive from multiple processes by creating a transient daemon process with buffer for each stream. (Surprisingly, this wasn't yet implemented).

Build:

make tarappend

Usage: 

some_program | tarappend  tar_file_name tar_element_name_for_this_process &

 

