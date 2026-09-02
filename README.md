# Windows CMD Utilities
A repository containing small scripts or utility executables for Windows cmd console.

## How to run
1. Simply download the repo (As a zip if you want to, then unzip). As an example, folder structure is shown below as is on my PC.

<img width="500" height="auto" alt="image" src="https://github.com/user-attachments/assets/717a1c53-bd9c-4e3c-ad49-25aeae4b59c9" />

2. Copy the folder path (in my case that's `C:\Aliases`) to your environment PATH variable as shown.
3. A new cmd window should ideally load the utilities to be used, else a reboot will definitely do it.

<details>
<Summary> <h3>EXPAND TO VIEW ALL THE STEPS NEEDED TO ADD THE FOLDER TO PATH</h3> </Summary>
  
- Search for env in start and open the result as shown below.
  - <img width="400" height="auto" alt="image" src="https://github.com/user-attachments/assets/390462c6-a98b-462b-9ef7-d37cd8a9d51b" />
- click on the button marked in the image below.
  - <img width="400" height="auto" alt="image" src="https://github.com/user-attachments/assets/8d5f8228-9c58-4c56-8e95-b13abe4295c3" />
- Select the entry called `Path` as shown in the image below, then click the edit button. Ideally do this on the first panel (which is for your user account, and not the bottom which is system wide)
  - <img width="600" height="auto" alt="image" src="https://github.com/user-attachments/assets/c245c07f-c46c-4443-b4f8-1251f9d45c0a" />
- This will open up this window, where you can click on `New` button and then enter the path to your folder which is in my case `C:\Aliases`
  - <img width="600" height="auto" alt="image" src="https://github.com/user-attachments/assets/dfcb75cd-837a-4f52-be31-11a42e9959b6" />
- Select the new entry and click the move up button, till it's at the top (will require multiple clicks).
- Then simply click okay on all the dialog boxes and windows.
</details>


**Note:**
- I am putting the exe in the repo itself since they are tiny and that means anyone downloading it gets it from a single place.
- Where the it's an executable, the C++ script is attached as well.
- These has only been tested on my PC which is Windows 11 Enterprise 24H2 LTSC version, and while unlikely, may not work on your machine.
- Feel free to post an issue if anything doesn't work, but a quick fix is not guaranteed.


---

# List of scripts and executable.

## listcmd.exe

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

## refreshenv.bat 

**Run it by simply typing `refreshenv`** 

The refreshenv refreshes system ENV and PATHS for windows cmd in the same session, taken from Chocolatey but is a modified version that fixes a bug present in choco's version.

- The difference being that it fixed a bug where the choco's native refreshenv would erase volatile Paths. (a PR remains waiting to be merged upstream)
- The difference from my PR version (where the bug has been fixed) higher includes the removal of WMIC here since it doesnt really exist on Win 24H2 and higher.


## runver.exe

**Run it as `runver N executable`, such as `runver 2 python`**

- When more than one executable (binary or script whatever) are present on PATH, the first one is executed by default on windows.
- If you want to run the other ones you will have to type to full path to that executable.
- An example would be having two python for example, in my case, that's python 3.10 and 3.11 and the python 3.11 is the one executed by calling python launches python 3.11
- **Runver let's you run the second executable by simply calling it `runver 2 python`**

Example:

```
> where python
C:\Program Files\Python311\python.exe
C:\Users\Rev Oconner\AppData\Local\Programs\Python\Python310\python.exe

> runver 2 python --version
Python 3.10.11

> runver 1 python --version
Python 3.11.9

> runver list nasm
1  C:\Users\Rev Oconner\AppData\Local\bin\NASM\nasm.exe
2  C:\Users\Rev Oconner\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.MCF.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin\nasm.exe
```

Help:

```
runver
runver - run the Nth program of a given name found on PATH

usage: runver <n> <program> [args...]
       runver list <program>

  <n>      1-based index in cmd's search order, so 1 is what typing
           the bare name would run, 2 the first shadowed one, and so on
  list     show every match with its index, in search order

example: runver list python
         runver 2 python -c "import sys; print(sys.version)"
```

### Important note
- Around 2001, there used to be a malware called runver.exe. So some niche antivirus might end up flagging it as one simply because it's an exe with the same name. It got flagged on virus total by 4 vendors. 
- Link to the run as shown [virustotal scan](https://www.virustotal.com/gui/file/ee58096778ea56aff04218ebfdaaea9ba5883d72cf5e17de772b32db122a5241)

<img width="350" height="auto" alt="image" src="https://github.com/user-attachments/assets/715e2f27-ad18-4256-aebe-fd21847c666d" />


## unblock.bat

**Run it as `unblock`**
- Removes the "This file came from another computer and might be blocked to help protect this computer" (mark of the web) on a downloaded file interactively using powershell being the scene.
- Does a recursive unblock on a path or non recursive unblock depending on your choice.
- To run in the folder you are in simply use `.`

<img width="553" height="auto" alt="image" src="https://github.com/user-attachments/assets/df7b4fee-20bc-4da7-b598-8d0f097ce521" />

Example:

```
> unblock
Paste the folder path: .

1. Unblock all files recursively (includes subfolders)
2. Unblock files in that folder only

Select option (1 or 2): 1
```


## vercel-rm.bat
**Requires Vercel CLI installed and logged in to work**
**Run as `vercel-rm website` where website is the project name**

- Deletes multiple entries of vercel deployment to cleanup your dashboard.
- Keep the latest entry from each branch (git deployments).
