# 📄 **lprun --- A Modern Command-Line Printer Utility**

`lprun` is a lightweight, fast, and extensible **command-line printing
tool** written in C.\
It acts as a smarter alternative to traditional UNIX tools like `lp` and
`lpr`, while adding:

-   automatic network printer discovery\
-   USB printer detection\
-   simple text & image printing\
-   CUPS integration\
-   a clean plugin-ready architecture

`lprun` is designed for Linux users who want full control of their
printers from the terminal, without relying on heavy GUI utilities.

------------------------------------------------------------------------

## 🚀 Features

-   🔍 **Auto-detect network printers** using:

    -   ARP scanning\
    -   CUPS browsing\
    -   JetDirect port detection (9100)

-   🔌 **Automatic USB printer detection**

-   🖨 **Print text or images easily**:

    ``` bash
    lprun --text "Hello world"
    lprun --image ~/photo.png
    ```

-   📡 **Print directly over port 9100 (raw JetDirect)**

-   🧠 **CUPS-backed printing support**:

    ``` bash
    lprun --file document.pdf --printer "Canon G3020 series"
    ```

-   🧱 Clean, modular C project structure:

        src/
          disc.c
          print.c
          util.c
        include/
          disc.h
          print.h
          util.h

-   🛠 Build using standard Makefile or CMake

------------------------------------------------------------------------

## 📦 Build & Installation

### **Using CMake**

``` bash
mkdir build
cd build
cmake ..
make
sudo make install
```

------------------------------------------------------------------------

## 🧑‍💻 Usage

### **List detected printers**

``` bash
lprun --list
```

### **Print simple text**

``` bash
lprun --text "Hello world"
```

### **Print a text file**

``` bash
lprun --file notes.txt
```

### **Print image**

``` bash
lprun --image picture.png
```

### **Specify a printer**

``` bash
lprun --printer "Canon G3020 series" --file report.pdf
```

### **Raw JetDirect printing**

``` bash
lprun --raw --host 192.168.1.50 --file mydoc.txt
```

------------------------------------------------------------------------

## ⚙️ Command-Line Arguments

  Option                Description
  --------------------- ---------------------------------------
  `--discover`          Scan local network + USB for printers
  `--printer <name>`    Choose printer manually
  `--text "<string>"`   Print inline text
  `--file <path>`       Print any file
  `--image <path>`      Convert + print PNG/JPG images
  `--copies N`          Number of copies
  `--raw`               Send raw data directly to printer
  `--host <IP>`         Printer IP (JetDirect mode)
  `--help`              Show help

------------------------------------------------------------------------

## 🧩 Project Structure

    lprun/
    │
    ├── src/
    │   ├── lprun.c
    │   ├── disc.c
    │   ├── print.c
    │   ├── util.c
    │
    ├── include/
    │   ├── disc.h
    │   ├── print_raw.h
    │   ├── print_cups.h
    │   ├── util.h
    │
    ├── README.md
    ├── Makefile
    └── CMakeLists.txt

------------------------------------------------------------------------

## 🛡 Dependencies

-   C standard library
-   CUPS development libraries\
    (`libcups` + `cups-devel`)
-   libc / GNU extensions (`_GNU_SOURCE`)
-   Optional: NetPBM or ImageMagick for PNG/JPG → raster conversion

Arch Linux:

``` bash
sudo pacman -S cups cups-filters libcups netpbm
```

Debian/Ubuntu:

``` bash
sudo apt install libcups2-dev cups-filters
```

------------------------------------------------------------------------

## 🧪 Example (Print Raw to Port 9100)

``` bash
echo -e "TEST PAGE\n" | lprun --raw --host 192.168.1.50
```

------------------------------------------------------------------------

## 📘 Future Improvements

-   PDF → raster conversion inside lprun
-   support for ESC/POS receipt printers
-   auto-install CUPS drivers
-   QR code generation
-   full scanning module

------------------------------------------------------------------------

## 📝 License

MIT License

------------------------------------------------------------------------

## 🤝 Contributing

Pull requests are welcome!
