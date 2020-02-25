# logtrunc
Similar to `logrotate` but in a simplified way by truncating logs.

# Features
- Watch logfile with `inotify` to not sleep specified amount of time
  - This prevent burst write to exceed maxsize and not be triggered
- Stop and restart specified service while truncating the file

# Notes
This software is made for specific purpose running on [Zero-OS](https://github.com/threefoldtech/zos), but
code and feature can be reuse for others purpose.
