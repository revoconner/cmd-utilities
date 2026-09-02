# Windows CMD Utilities
A repository containing small scripts or utility executables for Windows cmd console.

## How to run
- Simply download the repo (As a zip if you want to, then unzip)
  - As an example of how the folder structure is on my PC.  
  - <img width="500" height="auto" alt="image" src="https://github.com/user-attachments/assets/717a1c53-bd9c-4e3c-ad49-25aeae4b59c9" />
- Copy the folder path (in my case that's `C:\Aliases`) to your environment PATH variable as shown.
  - Search for env in start and open the result as shown below.
  - <img width="400" height="auto" alt="image" src="https://github.com/user-attachments/assets/390462c6-a98b-462b-9ef7-d37cd8a9d51b" />
  - click on the button marked in the image below.
  - <img width="400" height="auto" alt="image" src="https://github.com/user-attachments/assets/8d5f8228-9c58-4c56-8e95-b13abe4295c3" />
  - Select the entry called `Path` as shown in the image below, then click the edit button. Ideally do this on the first panel (which is for your user account, and not the bottom which is system wide)
  - <img width="600" height="auto" alt="image" src="https://github.com/user-attachments/assets/c245c07f-c46c-4443-b4f8-1251f9d45c0a" />
  - This will open up this window, where you can click on `New` button and then enter the path to your folder which is in my case `C:\Aliases`
  - <img width="600" height="auto" alt="image" src="https://github.com/user-attachments/assets/dfcb75cd-837a-4f52-be31-11a42e9959b6" />
  - Then simply click okay on all the dialog boxes and windows.


## List of scripts and executable.

Note:
- I am putting the exe in the repo itself since they are tiny and that means anyone downloading it gets it from a single place.
- Where the it's an executable, the C++ script is attached as well.
- These has only been tested on my PC which is Windows 11 Enterprise 24H2 LTSC version, and while unlikely, may not work on your machine.
- Feel free to post an issue if anything doesn't work, but a quick fix is not guaranteed.


---

### refreshenv.bat 
**Run it by simply typing `refreshenv`** 

The refreshenv refreshes system ENV and PATHS for windows cmd in the same session, taken from Chocolatey but is a modified version that fixes a bug present in choco's version.

- The difference being that it fixed a bug where the choco's native refreshenv would erase volatile Paths. (a PR remains waiting to be merged upstream)
- The difference from my PR version (where the bug has been fixed) higher includes the removal of WMIC here since it doesnt really exist on Win 24H2 and higher.

### listcmd.exe
**Run it by simply typing `listcmd` or with `listcmd --details` etc.
Displays all commands available in your windows cmd installation. Pretty useful if you ever find yourself in winPE or something. 

```
listcmd - list everything callable from Windows cmd
usage: listcmd [--details | --dup | --help]
  (no args)  unique callable names only: builtins, doskey macros, and the
             PATH file that wins bare-name resolution for each name
  --details  full report: builtins, macros, and every PATH file with its
             location and a SHADOWED tag
  --dup      only names present in more than one PATH directory, each with
             its count and every path, in PATH resolution order
  --help     this help
```

### runver
