# Linux Kernel Message Slot Module

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](./LICENSE)
[![Kernel](https://img.shields.io/badge/Linux%20Kernel-5.x%2B-orange.svg)]()
[![Language](https://img.shields.io/badge/Language-C-blue.svg)]()

> A Linux kernel module implementing a character device driver for inter-process communication (IPC) via message slots with multiple concurrent channels.

## ⚠️ ACADEMIC INTEGRITY WARNING

**THIS REPOSITORY IS FOR PORTFOLIO AND EDUCATIONAL PURPOSES ONLY.**

If you are a student currently enrolled in an Operating Systems course or any course with similar kernel programming assignments, **copying or referencing this code violates academic integrity policies**.

**I DO NOT take responsibility for any disciplinary actions taken against individuals who misuse this code.**

Potential consequences of academic misconduct include:
- Course failure (grade of 250 or "did not complete course requirements")
- Academic probation
- Expulsion from your academic institution
- Permanent record of dishonesty

**This code is meant to be reviewed by employers, educators, and researchers - NOT copied by students for coursework.**

See the [LICENSE](./LICENSE) file for the full Academic Integrity Notice.

---

## 🎯 Project Overview

This project implements a Linux kernel module that provides a new IPC mechanism called **message slots**. A message slot is a character device file through which processes communicate using multiple concurrent message channels.

### Key Features

- **Multiple Message Slots**: Each device file (identified by minor number) is an independent message slot
- **Multiple Channels per Slot**: Each slot supports up to 2^20 concurrent message channels
- **Persistent Messages**: Messages remain in channels until overwritten (unlike pipes)
- **Atomic Operations**: Read/write operations are atomic - entire messages are transferred
- **Optional Censorship**: Per-file-descriptor censorship mode (replaces every 4th character with '#')
- **Concurrent Access**: Multiple processes can use the same slot and channels
- **Thread-Safe Design**: Proper synchronization prevents race conditions

---

## 📚 Technical Deep Dive

### Understanding IOCTLs (Input/Output Control)

**What are IOCTLs?**

IOCTLs are a system call interface for device-specific operations that don't fit the standard read/write/open/close model. They allow user-space programs to send commands and data to device drivers.

**Why use IOCTLs in this project?**

Message slots need configuration operations beyond simple I/O:
- **Channel Selection**: Set which channel to read/write
- **Censorship Mode**: Enable/disable message censorship

These operations don't involve data transfer, so read/write wouldn't be appropriate.

**IOCTL Implementation Details:**

1. **MSG_SLOT_CHANNEL** (`_IOW(235, 0, unsigned int)`):
   - **Purpose**: Associates a channel ID with a file descriptor
   - **Parameter**: Non-zero unsigned integer (channel ID)
   - **Effect**: All subsequent reads/writes on this FD use this channel
   - **Design Rationale**: Allows multiple processes to use different channels on the same device file

2. **MSG_SLOT_SET_CEN** (`_IOW(235, 1, unsigned int)`):
   - **Purpose**: Sets censorship mode for this file descriptor
   - **Parameter**: 0 (disabled) or 1 (enabled)
   - **Effect**: When enabled, every 4th character in written messages becomes '#'
   - **Design Rationale**: Demonstrates per-FD state management in kernel

**IOCTL Command Encoding:**

```c
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned int)
```

The `_IOW` macro encodes:
- **Direction**: Write (data flows from user to kernel)
- **Major Number**: Device identifier (235)
- **Command Number**: Unique command ID (0)
- **Data Type**: unsigned int (ensures type safety)

### Race Condition Prevention

**What are Race Conditions?**

Race conditions occur when multiple threads/processes access shared data concurrently, and the outcome depends on the timing of execution. In kernel code, race conditions can cause:
- Data corruption
- System crashes
- Security vulnerabilities

**Race Condition Scenarios in Message Slots:**

1. **Concurrent Channel Creation**:
   - **Problem**: Two processes call `get_channel()` simultaneously for the same channel
   - **Risk**: Both might allocate a new channel, causing memory leak
   - **Solution**: The assignment assumes no concurrent operations (single-threaded access)

2. **Concurrent Read/Write**:
   - **Problem**: One process reads while another writes to the same channel
   - **Risk**: Reader might get partial/corrupted message
   - **Solution**: Assignment guarantees no concurrent system calls

3. **Module Unload During Use**:
   - **Problem**: Module unloaded while file descriptors are still open
   - **Risk**: Kernel crash when operations reference freed memory
   - **Solution**: `.owner = THIS_MODULE` in `file_operations` prevents premature unloading

**Synchronization Techniques (Not Required Here):**

While this assignment assumes no concurrency, production code would use:
- **Spinlocks**: For short critical sections (e.g., updating channel list)
- **Mutexes**: For longer critical sections (e.g., allocating memory)
- **RCU (Read-Copy-Update)**: For read-heavy workloads
- **Atomic Operations**: For simple counter updates

**Why This Assignment Doesn't Require Locking:**

The assignment specification states:
> "You can assume that any invocation of the module's operations (including loading/unloading) will run alone; i.e., there will not be concurrent system call invocations."

This simplifies the implementation for educational purposes, focusing on:
- Device driver architecture
- Memory management
- User-space/kernel-space data transfer
- IOCTL command handling

**Real-World Considerations:**

In production kernel modules, you would need:
```c
static DEFINE_SPINLOCK(slots_lock);

static channel_t *get_channel(slot_t *slot, unsigned int id) {
    channel_t *c;
    unsigned long flags;

    spin_lock_irqsave(&slots_lock, flags);
    // Critical section: search/create channel
    spin_unlock_irqrestore(&slots_lock, flags);

    return c;
}
```

---

## 🏗️ Architecture

### Data Structures

```
Global: slots (linked list of all message slots)
    ↓
slot_t (one per device minor number)
  ├── minor: device minor number
  ├── channels: linked list of channels
  └── next: next slot
      ↓
  channel_t (one per channel ID)
    ├── id: channel identifier
    ├── message: stored message data
    ├── len: message length
    └── next: next channel
```

### Memory Management

**Allocation Strategy:**
- **Lazy Allocation**: Slots and channels are created on first access
- **Kernel Memory**: Uses `kmalloc(GFP_KERNEL)` for all allocations
- **No Limits**: No hard limit on number of slots/channels (within memory constraints)

**Memory Complexity:**
- **Space**: O(C * M + N)
  - C = number of channels
  - M = size of largest message
  - N = number of file descriptors (for fd_state)

**Cleanup:**
- Module unload frees all memory (slots → channels → messages)
- No per-FD cleanup needed (file->private_data freed by kernel)

### File Operations Flow

**Opening a Device:**
```
device_open() → allocate fd_state → store in file->private_data
```

**Setting Up Communication:**
```
ioctl(MSG_SLOT_SET_CEN, mode) → set state->censor
ioctl(MSG_SLOT_CHANNEL, id) → set state->channel_id
```

**Writing a Message:**
```
device_write() → copy from user → apply censorship →
  get_slot() → get_channel() → allocate/replace message → store
```

**Reading a Message:**
```
device_read() → get_slot() → get_channel() → copy to user
```

---

## 🚀 Building and Testing

### Prerequisites

- Linux kernel headers for your running kernel
- GCC compiler
- Root access (for module loading and device file creation)

### Build Instructions

```bash
cd Linux_Kernel_Module
make all
```

This builds:
- `message_slot.ko` - Kernel module
- `message_sender` - User-space sender program
- `message_reader` - User-space reader program

### Installation and Testing

**1. Load the Module:**
```bash
sudo insmod message_slot.ko
```

**2. Create Device Files:**
```bash
sudo mknod /dev/slot0 c 235 0
sudo mknod /dev/slot1 c 235 1
sudo chmod 666 /dev/slot0 /dev/slot1
```

**3. Send a Message (No Censorship):**
```bash
./message_sender /dev/slot0 1 0 "Hello, Kernel!"
```
- Device: `/dev/slot0`
- Channel: `1`
- Censorship: `0` (disabled)
- Message: `"Hello, Kernel!"`

**4. Read the Message:**
```bash
./message_reader /dev/slot0 1
```
Output: `Hello, Kernel!`

**5. Send with Censorship:**
```bash
./message_sender /dev/slot0 1 1 "ABCDEFGHIJKLMNOP"
```
- Censorship: `1` (enabled)
- Every 4th character (indices 3, 7, 11, 15) becomes '#'

**6. Read Censored Message:**
```bash
./message_reader /dev/slot0 1
```
Output: `ABC#EFG#IJK#MNO#`

**7. Test Multiple Channels:**
```bash
./message_sender /dev/slot0 1 0 "Channel 1"
./message_sender /dev/slot0 2 0 "Channel 2"
./message_sender /dev/slot0 3 0 "Channel 3"

./message_reader /dev/slot0 1  # Outputs: Channel 1
./message_reader /dev/slot0 2  # Outputs: Channel 2
./message_reader /dev/slot0 3  # Outputs: Channel 3
```

**8. Unload the Module:**
```bash
sudo rmmod message_slot
```

### Troubleshooting

**Error: "No such file or directory" for kernel headers:**
```bash
sudo apt-get update
sudo apt-get install linux-headers-$(uname -r)
```

**Error: "Device or resource busy" when unloading:**
- Close all open file descriptors to the device files
- Use `lsof | grep slot` to find processes using the device

**Error: "Operation not permitted" when creating device files:**
- Ensure you're using `sudo`
- Verify the device files don't already exist

---

## 📂 Project Structure

```
Kernel-Development-Lab/
├── Linux_Kernel_Module/          # Main module directory
│   ├── message_slot.c            # Kernel module implementation
│   ├── message_slot.h            # Header with IOCTL definitions
│   ├── message_sender.c          # User-space sender program
│   ├── message_reader.c          # User-space reader program
│   ├── Makefile                  # Build system
│   └── instructions_kernel.txt   # Original assignment specification
├── README.md                     # This file
├── INTERVIEW_NOTES.md            # Technical decision documentation
├── LICENSE                       # MIT License + Academic Integrity Notice
└── .github/
    └── copilot-instructions.md   # Coding standards for AI assistance
```

---

## 🧠 Key Concepts Demonstrated

### Kernel Programming
- Character device driver implementation
- Module initialization and cleanup
- File operations (`open`, `read`, `write`, `ioctl`)
- Kernel memory management (`kmalloc`, `kfree`)
- User-space ↔ kernel-space data transfer (`get_user`, `put_user`)

### IPC Mechanisms
- Multiple communication channels over a single device
- Persistent message storage (vs. transient pipes)
- Per-file-descriptor state management

### Software Engineering
- Clean, documented code with Doxygen comments
- Error handling and validation
- Memory management without leaks
- Professional code organization

---

## 🎓 Learning Resources

- [Linux Kernel Module Programming Guide](https://sysprog21.github.io/lkmpg/)
- [Linux Device Drivers (LDD3)](https://lwn.net/Kernel/LDD3/)
- [Understanding IOCTL](https://www.kernel.org/doc/html/latest/driver-api/ioctl.html)
- [Kernel Space vs User Space](https://www.redhat.com/sysadmin/user-kernel-space)

---

## 📝 License

This project is licensed under the MIT License with an Academic Integrity Notice.

**Copyright (c) 2026 Odeliya Cohen**

See [LICENSE](./LICENSE) for full terms and the Academic Integrity Notice.

---

## 👤 Author

**Odeliya Cohen**

This project was completed as part of Operating Systems coursework and demonstrates:
- Linux kernel programming expertise
- Understanding of IPC mechanisms
- Clean code practices and documentation
- Systems programming skills

---

## 🤝 Contributing

This is a portfolio project and not accepting contributions. However, feel free to:
- Fork for learning purposes (after completing your own coursework)
- Report bugs or suggest improvements via issues
- Use as a reference for understanding kernel modules

---

**Note**: This README provides comprehensive documentation for technical interviews and portfolio reviews. For the original assignment specification, see `Linux_Kernel_Module/instructions_kernel.txt`.
