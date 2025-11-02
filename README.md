# Pintos OS - Installation Guide

## Installation

### 1. Obtain Pintos Source Code

```bash
git clone https://github.com/jhu-cs318/pintos.git
cd pintos
```

### 2. Check Existing Toolchain

Verify if your system already supports 32-bit x86 architecture:

```bash
objdump -i | grep elf32-i386
```

If this command produces output, you can skip to [Step 4 (Emulator)](#4-install-emulator). Otherwise, proceed to build the toolchain from source.

### 3. Build Compiler Toolchain from Source

#### Install Prerequisites

```bash
sudo apt-get install build-essential automake git
sudo apt-get install libncurses5-dev texinfo
```

#### Build the Toolchain

```bash
SWD=/path/to/setup  # Replace with your desired path, e.g., /home/username/toolchain
mkdir -p $SWD
cd /path/to/pintos/src
misc/toolchain-build.sh $SWD
```

#### Update PATH Environment Variable

Add the following line to your `~/.bashrc` file:

```bash
export PATH=$SWD/x86_64/bin:$PATH
```

Restart your terminal or run:

```bash
source ~/.bashrc
```

#### Verify Installation

```bash
which i386-elf-gcc
i386-elf-gcc --version  # Should show version 6.2.0
```

### 4. Install Emulator

Install QEMU for running Pintos:

```bash
sudo apt-get install qemu libvirt-bin
```

### 5. Setup Pintos Utility Tools

Build and install the Pintos utilities:

```bash
cd pintos/src/utils && make
cp backtrace pintos Pintos.pm pintos-gdb pintos-set-cmdline \
   pintos-mkdisk setitimer-helper squish-pty squish-unix \
   /path/to/swd/x86_64/bin
mkdir /path/to/swd/x86_64/misc
cp ../misc/gdb-macros /path/to/swd/x86_64/misc
```

## Additional Requirements

### Required
- Perl 5.8.0 or later

### Recommended
- **cgdb** - Enhanced debugger interface (strongly recommended)
- **ctags** - Code navigation
- **cscope** - Code browsing
- **NERDTree** - File explorer for Vim
- **YouCompleteMe** - Code completion

### Optional
- GUI editors (VS Code, CLion, etc.)

## Running Pintos

Test your installation:

```bash
cd pintos/src/threads
make
cd build
pintos --
```

### Expected Output

If successful, you should see:

```
PiLo hda1
Loading...........
Kernel command line:
Pintos booting with 3,968 kB RAM...
367 pages available in kernel pool.
367 pages available in user pool.
Calibrating timer... 838,041,600 loops/s.
Boot complete.
```

## Troubleshooting

### "command not found" error for i386-elf-gcc
- Ensure you added the export PATH line to `~/.bashrc`
- Restart your terminal or run `source ~/.bashrc`
- Verify the path is correct with `echo $PATH`

### Build failures
- Ensure all prerequisites are installed
- Check that you have sufficient disk space
- Verify you're using a compatible Ubuntu version

## Project Structure

```
pintos/
├── src/
│   ├── threads/    # Thread implementation
│   ├── userprog/   # User program support
│   ├── vm/         # Virtual memory
│   ├── filesys/    # File system
│   ├── utils/      # Utility scripts
│   └── misc/       # Miscellaneous tools
```

## Contributing

This is an educational project. Follow your course guidelines for submitting modifications and assignments.

## License

Refer to the original Pintos distribution for licensing information.

## Resources

- [Original Pintos Repository](https://github.com/jhu-cs318/pintos)
- Course: CS2042 - Operating Systems
