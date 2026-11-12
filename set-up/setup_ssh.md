# SSH Setup
SSH allows you to remotely access the Raspberry Pi running QNX from your computer.

## Before you start
Make sure:
- The Raspberry Pi is powered on.
- QNX 8.0 has finished booting.
- You have an Ethernet cable or access to the UW-Lab Wi-Fi network.
- You know the IP address of the Raspberry Pi you want to connect to. The Raspberry Pi's IP address is written on its case.

For this workshop, the default QNX username and password are:

Username: `qnxuser`

Password: `qnxuser`

## Step 1: Establish a Network Connection
### Option 1: Wi-Fi Connection

Connect your computer to the UW-Lab Wi-Fi network.

Password: LeZPjTCXW89hufgX

### Option 2: Ethernet Connection (If Wi-Fi Is Not Working)

If you cannot connect to the Raspberry Pi through Wi-Fi, you can use an Ethernet cable to establish a direct connection between your computer and the Raspberry Pi.

**Note:** If your computer does not have an Ethernet port, you will need a USB-to-Ethernet adapter.

1. Connect one end of the Ethernet cable to the Raspberry Pi's Ethernet port.
2. Connect the other end to your computer's Ethernet port. If you are using a USB-to-Ethernet adapter, connect the cable to the adapter and plug the adapter into your computer.
3. Wait for the Ethernet connection to establish.

## Step 2: Connect to Raspberry Pi via SSH

### Windows
Open VS Code and select Terminal.
Make sure the terminal is using PowerShell or Command Prompt.

Run:
```bash
ssh -m hmac-sha2-256 qnxuser@<Raspberry_Pi_IP>
```
For example, if your Raspberry Pi's IP address is 10.37.87.53, run:
```bash
ssh -m hmac-sha2-256 qnxuser@10.37.87.53
```
If this is the first time connecting to the Raspberry Pi, you may see:
```text
Are you sure you want to continue connecting (yes/no/[fingerprint])?
```
Enter:
```text
yes
```
When prompted for a password, enter:
```text
qnxuser
```
If the connection is successful, you should see:
```text
Welcome to QNX 8.0!
```
and a prompt similar to:
```text
qnxuser@qnxpi:~$
```

### Linux
Open VS Code and select Terminal.

Run:
```bash
ssh -m hmac-sha2-256 qnxuser@<Raspberry_Pi_IP>
```
For example, if your Raspberry Pi's IP address is 10.37.87.53, run:
```bash
ssh -m hmac-sha2-256 qnxuser@10.37.87.53
```
If this is the first time connecting to the Raspberry Pi, you may see:
```text
Are you sure you want to continue connecting (yes/no/[fingerprint])?
```
Enter:
```text
yes
```
When prompted for a password, enter:
```text
qnxuser
```
If the connection is successful, you should see:
```text
Welcome to QNX 8.0!
```


