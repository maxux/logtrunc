# logtrunc
Similar to `logrotate` but in a simplified way by truncating logs.

# Features
- Watch logfile with `inotify` to not sleep specified amount of time
  - This prevent burst write to exceed maxsize and not be triggered
- Stop and restart specified service while truncating the file
