# Navtor pre-interview tasks

# Snow White and the Dwarfs Party

This document calculates the number of dwarfs wearing orange hats at Snow White's party.

## Variables

Let:  

- **G** = number of dwarfs in green hats  
- **O** = number of dwarfs in orange hats  
- **R** = number of dwarfs in red hats  

**Total dwarfs at the party:**  

```math
G + O + R = 43
```
## Questions
- "Are you wearing a green hat?"
- "Are you wearing an orange hat?"
- "Are you wearing a red hat?"

## Dwarfs' Responses

- Green: (yes, no, no)  
- Orange: (yes, yes, yes) or (no, no, no)  
- Red: (yes, yes, no)  

## Equations

1. `G + R + (O - x) = 34`  
2. `R + (O - x) = 26`  
3. `O - x = 11`  

## Solution

```text
From equation (3):
O - x = 11

Substitute into equation (2):
R + 11 = 26
R = 15

Substitute into equation (1):
G + 15 + 11 = 34
G = 8

Calculate O using the total number of Dwarfs:
G + O + R = 43
8 + O + 15 = 43
O = 20
```

| Hat Color | Number of Dwarfs |
| --------- | ---------------- |
| Green     | 8                |
| Orange    | 20               |
| Red       | 15               |
| **Total** | 43               |

## Conclusion

20 Dwarfs were wearing orange hats at the party.

# 🕒 Back to the Future  
### Lightweight file compression and decompression tool for Windows (Console App)

Back to the Future is a simple yet efficient compression and decompression tool built in **C++** using **Visual Studio**.  
It uses a **custom compression algorithm**, designed for educational and experimental purposes — with test archives typically using the `.tmar` (Time Machine Archive) format.

---

## 🚀 Features

- Fast and lightweight command-line interface  
- Custom compression algorithm (not based on ZIP or other standard formats)  
- Packs **only unique file data blocks**, reducing archive size by eliminating duplicate content  
- During extraction, all **original files (including duplicates)** are fully reconstructed  
- Simple usage for both packing and unpacking directories  
- 100% native Windows executable 

---

## ⚙️ Requirements

- **Operating System:** Windows 10 or newer  
- **Compiler:** Built and tested with **Visual Studio (MSVC)**  
- **Runtime:** Requires the **Microsoft Visual C++ Redistributable**  
- No external libraries required  

---

## 💻 Usage

After downloading the executable from the [latest release](https://github.com/Shadow3677/Navtor/releases/latest),  
you can run the program directly from the console.

```bash
# Compress a folder into a .tmar archive
app pack <input_folder> <archive_path>

# Decompress a .tmar archive into a folder
app unpack <archive_path> <output_folder>
```

### Example

```bash
app pack "C:\Projects\GameAssets" "C:\Archives\game_assets.tmar"
app unpack "C:\Archives\game_assets.tmar" "C:\Extracted\GameAssets"
```

---

## 📦 Download

👉 Download the latest release [here](https://github.com/Shadow3677/Navtor/releases/latest)

## 🧰 Build from Source
If you prefer to build it yourself:

1. Clone the repository:
   ```bash
     git clone https://github.com/Shadow3677/Navtor.git
   ```
2. Open the project in Visual Studio
3. Build the Release configuration
4. The resulting executable will be available in the /x64/Release folder

---

## 🧑‍💻 Author

Łukasz Wiśniewski — C++ developer passionate about low-level systems.
Developed under Dark Paw Software 🐾

<p align="center"> <img src="https://i.imgur.com/P2pdHhL.png" alt="Dark Paw Software" width="220"/> </p>

---
