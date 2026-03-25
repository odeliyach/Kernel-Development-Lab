# Linux Kernel Message Slot Module

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](./LICENSE)
[![Kernel](https://img.shields.io/badge/Linux%20Kernel-5.x%2B-orange.svg)]()
[![Language](https://img.shields.io/badge/Language-C-blue.svg)]()

> A Linux kernel module implementing a character device driver for inter-process communication (IPC) via message slots with multiple concurrent channels.
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

## 🔧 System Architecture

This section explains how the message slot system's components interact, from user-space applications down to kernel data structures.

### High-Level Component Interaction

```
┌─────────────────────────────────────────────────────────────┐
│                      User Space                              │
│  ┌──────────────────┐              ┌──────────────────┐    │
│  │ message_sender   │              │ message_reader   │    │
│  │                  │              │                  │    │
│  │ 1. open()        │              │ 1. open()        │    │
│  │ 2. ioctl()       │              │ 2. ioctl()       │    │
│  │ 3. write()       │              │ 3. read()        │    │
│  │ 4. close()       │              │ 4. close()       │    │
│  └────────┬─────────┘              └────────┬─────────┘    │
└───────────┼────────────────────────────────┼───────────────┘
            │                                │
            │      System Call Interface     │
            │                                │
┌───────────┼────────────────────────────────┼───────────────┐
│           ▼         Kernel Space           ▼               │
│  ┌──────────────────────────────────────────────────────┐  │
│  │         Character Device Driver (message_slot)       │  │
│  │                                                      │  │
│  │  ┌──────────────┐  ┌────────────────────────────┐  │  │
│  │  │ device_open  │  │  device_ioctl              │  │  │
│  │  │              │  │  • MSG_SLOT_CHANNEL        │  │  │
│  │  │ Allocates    │  │  • MSG_SLOT_SET_CEN        │  │  │
│  │  │ fd_state     │  │                            │  │  │
│  │  └──────────────┘  │  Sets per-FD state:        │  │  │
│  │                    │  - channel_id               │  │  │
│  │  ┌──────────────┐  │  - censor mode             │  │  │
│  │  │ device_write │  └────────────────────────────┘  │  │
│  │  │              │                                   │  │
│  │  │ 1. copy_from_user()                            │  │
│  │  │ 2. Apply censorship (if enabled)               │  │
│  │  │ 3. get_slot() → get_channel()                  │  │
│  │  │ 4. Store message                               │  │
│  │  └──────┬───────┘                                  │  │
│  │         │                                          │  │
│  │         ▼                                          │  │
│  │  ┌────────────────────────────────────────────┐   │  │
│  │  │         Data Structure Layer              │   │  │
│  │  │                                           │   │  │
│  │  │  Global: slots (linked list)             │   │  │
│  │  │     │                                     │   │  │
│  │  │     ├─► slot_t (minor=0)                 │   │  │
│  │  │     │      └─► channel_t (id=1) ──► msg  │   │  │
│  │  │     │      └─► channel_t (id=2) ──► msg  │   │  │
│  │  │     │                                     │   │  │
│  │  │     └─► slot_t (minor=1)                 │   │  │
│  │  │            └─► channel_t (id=1) ──► msg  │   │  │
│  │  │                                           │   │  │
│  │  └────────────────────────────────────────────┘   │  │
│  │         ▲                                          │  │
│  │         │                                          │  │
│  │  ┌──────┴───────┐                                 │  │
│  │  │ device_read  │                                 │  │
│  │  │              │                                 │  │
│  │  │ 1. get_slot() → get_channel()                 │  │
│  │  │ 2. copy_to_user()                             │  │
│  │  └──────────────┘                                 │  │
│  └──────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
```

### System Call Deep Dive

#### 1. **IOCTL System Calls**

IOCTLs configure the behavior of file descriptors without transferring data. The message slot driver uses two IOCTL commands:

**MSG_SLOT_CHANNEL (`_IOW(235, 0, unsigned int)`)**
- **Purpose**: Associate a channel ID with the file descriptor
- **Flow**:
  1. User calls `ioctl(fd, MSG_SLOT_CHANNEL, channel_id)`
  2. Kernel validates `channel_id != 0`
  3. Stores `channel_id` in `file->private_data->channel_id`
  4. Returns 0 on success, -EINVAL on failure
- **Effect**: All subsequent read/write operations on this FD use this channel
- **Per-FD State**: Each file descriptor maintains its own channel selection

**MSG_SLOT_SET_CEN (`_IOW(235, 1, unsigned int)`)**
- **Purpose**: Enable/disable censorship mode for the file descriptor
- **Flow**:
  1. User calls `ioctl(fd, MSG_SLOT_SET_CEN, mode)` where mode is 0 or 1
  2. Kernel validates mode value
  3. Stores censorship flag in `file->private_data->censor`
  4. Returns 0 on success, -EINVAL on invalid mode
- **Effect**: When enabled, every 4th character (indices 3, 7, 11, ...) becomes '#' during write operations
- **Per-FD State**: Censorship is independent per file descriptor

**Why IOCTLs?**
- Configuration operations don't fit the read/write model
- Allows setting parameters without data transfer
- Type-safe: `_IOW` macro ensures correct parameter type
- Extensible: Easy to add new commands without breaking compatibility

#### 2. **WRITE System Call**

Writing a message involves data transfer from user space to kernel space and storage in channel data structures.

**Flow (`device_write`):**

```
User: write(fd, message, len)
  │
  ├─► Kernel Entry: device_write(file, buffer, len, offset)
  │
  ├─► [1] Validate File Descriptor State
  │      • Check: channel_id != 0 (must call ioctl first)
  │      • Return -EINVAL if no channel set
  │
  ├─► [2] Validate Message Length
  │      • Check: 0 < len ≤ MAX_MESSAGE_LEN (128 bytes)
  │      • Return -EMSGSIZE if invalid
  │
  ├─► [3] Copy from User Space
  │      • Allocate kernel buffer: kmalloc(len, GFP_KERNEL)
  │      • Return -ENOMEM if allocation fails
  │      • Copy data: copy_from_user(kernel_buf, buffer, len)
  │      • Return -EFAULT if copy fails (bad user pointer)
  │
  ├─► [4] Apply Censorship (if enabled)
  │      • If state->censor == 1:
  │      •   for i in {3, 7, 11, 15, ...}: kernel_buf[i] = '#'
  │
  ├─► [5] Navigate Data Structures
  │      • slot = get_slot(minor_number)
  │      •   Creates new slot if doesn't exist
  │      • channel = get_channel(slot, channel_id)
  │      •   Creates new channel if doesn't exist
  │
  ├─► [6] Store Message in Channel
  │      • If channel->message exists: kfree(old_message)
  │      • channel->message = kernel_buf
  │      • channel->len = len
  │
  └─► [7] Return Success
         • Return len (number of bytes written)
```

**Key Points:**
- **Atomic Operation**: Entire message stored or none of it (no partial writes)
- **Overwrite Semantics**: New message replaces old message in channel
- **Memory Safety**: User-space pointers never directly dereferenced
- **Error Handling**: All allocations and copies checked; cleanup on failure

#### 3. **READ System Call**

Reading retrieves a message from kernel space and transfers it to user space.

**Flow (`device_read`):**

```
User: n = read(fd, buffer, MAX_MESSAGE_LEN)
  │
  ├─► Kernel Entry: device_read(file, buffer, len, offset)
  │
  ├─► [1] Validate File Descriptor State
  │      • Check: channel_id != 0
  │      • Return -EINVAL if no channel set
  │
  ├─► [2] Navigate Data Structures
  │      • slot = get_slot(minor_number)
  │      • Return -EINVAL if slot doesn't exist
  │      • channel = get_channel(slot, channel_id)
  │      • Return -EINVAL if channel doesn't exist
  │
  ├─► [3] Check Message Existence
  │      • if channel->message == NULL:
  │      •   Return -EWOULDBLOCK (no message in channel)
  │
  ├─► [4] Validate Buffer Size
  │      • if len < channel->len:
  │      •   Return -ENOSPC (buffer too small)
  │
  ├─► [5] Copy to User Space
  │      • copy_to_user(buffer, channel->message, channel->len)
  │      • Return -EFAULT if copy fails (bad user pointer)
  │
  └─► [6] Return Success
         • Return channel->len (number of bytes read)
```

**Key Points:**
- **Non-Destructive Read**: Message remains in channel after reading
- **No Partial Reads**: User buffer must be large enough for entire message
- **Message Persistence**: Messages survive process termination
- **Error Codes**:
  - `-EINVAL`: Channel not set or doesn't exist
  - `-EWOULDBLOCK`: Channel exists but has no message
  - `-ENOSPC`: User buffer too small
  - `-EFAULT`: Bad user-space pointer

### Data Structure Interaction

**Hierarchical Storage:**

```
1. File Descriptor (User Space)
   │
   ├─► file->private_data (Per-FD State)
   │     ├─► channel_id: Which channel this FD uses
   │     └─► censor: Whether censorship is enabled
   │
   └─► iminor(file->f_inode) (Device Minor Number)
         │
         ├─► Determines which slot_t to use
         │
         └─► slot_t (Kernel Space - Per Device)
               ├─► minor: Device minor number
               ├─► channels: Linked list of channels
               │
               └─► channel_t (Per Channel ID)
                     ├─► id: Channel identifier
                     ├─► message: Stored message buffer
                     ├─► len: Message length
                     └─► next: Next channel in list
```

**Lookup Process:**

1. **File Descriptor → Minor Number**
   - Extract: `minor = iminor(file->f_inode)`
   - Identifies which device file (e.g., /dev/slot0 vs /dev/slot1)

2. **Minor Number → Slot**
   - Search global `slots` linked list
   - Match: `slot->minor == minor`
   - Create new slot if not found (lazy allocation)

3. **Channel ID → Channel**
   - Search `slot->channels` linked list
   - Match: `channel->id == channel_id`
   - Create new channel if not found (lazy allocation)

4. **Channel → Message**
   - Direct access: `channel->message` and `channel->len`
   - Stored in kernel heap (allocated with `kmalloc`)

**Memory Lifecycle:**

```
Module Load
  │
  ├─► slots = NULL (empty global list)
  │
First write() to /dev/slot0, channel 1
  │
  ├─► get_slot(0)
  │     └─► Creates slot_t with minor=0
  │           └─► Adds to global slots list
  │
  ├─► get_channel(slot, 1)
  │     └─► Creates channel_t with id=1
  │           └─► Adds to slot->channels list
  │
  ├─► Allocates message buffer
  │     └─► channel->message = kmalloc(len)
  │
Subsequent write() to same channel
  │
  ├─► Finds existing slot and channel
  │
  ├─► Frees old message: kfree(channel->message)
  │
  └─► Allocates new message: channel->message = kmalloc(new_len)

Module Unload
  │
  ├─► For each slot in slots:
  │     ├─► For each channel in slot->channels:
  │     │     ├─► kfree(channel->message)
  │     │     └─► kfree(channel)
  │     └─► kfree(slot)
  │
  └─► slots = NULL
```

### Concurrency and Safety Considerations

**Single-Threaded Assumption:**
This implementation assumes no concurrent system calls (per assignment specification). In production, you would need:

```c
// Production example (not required for this assignment):
static DEFINE_SPINLOCK(slots_lock);
static DEFINE_SPINLOCK(channel_lock);

static ssize_t device_write(...) {
    unsigned long flags;

    spin_lock_irqsave(&slots_lock, flags);
    // Critical section: access/modify slots and channels
    spin_unlock_irqrestore(&slots_lock, flags);

    return len;
}
```

**Memory Safety:**
- ✅ **No Direct User Pointer Dereference**: Always use `copy_from_user`/`copy_to_user`
- ✅ **Allocation Checks**: All `kmalloc` calls checked before use
- ✅ **Buffer Overflow Prevention**: Length validated before copy operations
- ✅ **Reference Counting**: `.owner = THIS_MODULE` prevents module unload during use

**Error Handling Strategy:**
- **Validate Early**: Check all preconditions before allocation
- **Clean Up on Failure**: Free partial allocations on error paths
- **Return Standard Errno**: Use standard error codes (-EINVAL, -ENOMEM, etc.)
- **Atomic Operations**: Either complete successfully or leave no side effects

---

## 🚀 Building and Testing

### Prerequisites

- Linux kernel headers for your running kernel
- GCC compiler
- Root access (for module loading and device file creation)

### Build Instructions

```bash
# Build everything (kernel module and user applications)
make

# Or build individual components:
make module        # Build only the kernel module
make user_programs # Build only the user applications

# Clean all build artifacts
make clean

# Show help
make help
```

This builds:
- `message_slot.ko` - Kernel module (in root directory)
- `user_apps/message_sender` - User-space sender program
- `user_apps/message_reader` - User-space reader program

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
./user_apps/message_sender /dev/slot0 1 0 "Hello, Kernel!"
```
- Device: `/dev/slot0`
- Channel: `1`
- Censorship: `0` (disabled)
- Message: `"Hello, Kernel!"`

**4. Read the Message:**
```bash
./user_apps/message_reader /dev/slot0 1
```
Output: `Hello, Kernel!`

**5. Send with Censorship:**
```bash
./user_apps/message_sender /dev/slot0 1 1 "ABCDEFGHIJKLMNOP"
```
- Censorship: `1` (enabled)
- Every 4th character (indices 3, 7, 11, 15) becomes '#'

**6. Read Censored Message:**
```bash
./user_apps/message_reader /dev/slot0 1
```
Output: `ABC#EFG#IJK#MNO#`

**7. Test Multiple Channels:**
```bash
./user_apps/message_sender /dev/slot0 1 0 "Channel 1"
./user_apps/message_sender /dev/slot0 2 0 "Channel 2"
./user_apps/message_sender /dev/slot0 3 0 "Channel 3"

./user_apps/message_reader /dev/slot0 1  # Outputs: Channel 1
./user_apps/message_reader /dev/slot0 2  # Outputs: Channel 2
./user_apps/message_reader /dev/slot0 3  # Outputs: Channel 3
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
├── src/                          # Kernel module source code
│   ├── message_slot.c            # Kernel module implementation
│   └── Makefile                  # Kernel build configuration
├── include/                      # Header files
│   └── message_slot.h            # IOCTL definitions and constants
├── user_apps/                    # User-space applications
│   ├── message_sender.c          # Message sender program
│   └── message_reader.c          # Message reader program
├── docs/                         # Documentation
│   ├── instructions_kernel.txt   # Original assignment specification
│   └── DESIGN_DECISIONS.md       # Architecture and design notes
├── tests/                        # Test files and scripts
│   └── README.md                 # Testing documentation
├── Makefile                      # Root build system (builds everything)
├── README.md                     # This file
├── INTERVIEW_NOTES.md            # Technical decision documentation
├── LICENSE                       # MIT License + Academic Integrity Notice
├── Dockerfile                    # Container for development/testing
└── .github/
    └── copilot-instructions.md   # Coding standards for AI assistance
```

### Directory Purposes

- **`src/`**: Contains the kernel module source code (`message_slot.c`) and its build configuration
- **`include/`**: Shared header files used by both kernel module and user applications
- **`user_apps/`**: User-space programs for interacting with the message slot device
- **`docs/`**: Project documentation, specifications, and design decisions
- **`tests/`**: Test files and testing documentation

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

## ⚠️ Academic Integrity Notice

**This repository is for portfolio and educational purposes only.** If you are a student currently taking an Operating Systems course, copying this code violates academic integrity policies. I do not take responsibility for disciplinary actions against individuals who misuse this code.
See the [LICENSE](./LICENSE) file for the full Academic Integrity Notice.

---
## 📝 License

This project is licensed under the MIT License with an Academic Integrity Notice.

**Copyright (c) 2026 Odeliya Caritonova**

See [LICENSE](./LICENSE) for full terms and the Academic Integrity Notice.



**Note**: This README provides comprehensive documentation for technical interviews and portfolio reviews. For the original assignment specification, see `docs/instructions_kernel.txt`.

---
<div align="center">

Built by **Odeliya Charitonova** · [GitHub](https://github.com/odeliyach) · [LinkedIn](https://linkedin.com/in/odeliya-charitonova)

*Computer Science student @ Tel Aviv University, School of CS & AI*

</div>
