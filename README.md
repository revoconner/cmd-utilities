# Windows CMD Utilities
A repository containing small scripts or utility executables for Windows cmd console.

## Description of each item and help is available in the [wiki](https://github.com/revoconner/cmd-utilities/wiki/Description-of-all-utilities)

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


