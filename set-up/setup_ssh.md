# SSH Setup
SSH allows you to remotely access the Raspberry Pi running QNX from your computer.

## Before you start
Make sure:
- The Raspberry Pi is powered on.
- QNX 8.0 has finished booting.
- Your computer and the Raspberry Pi are connected to the same network.
- You know the Raspberry Pi IP address.

For this workshop, the default QNX username and password are:

Username: `qnxuser`

Password: `qnxuser`

## Windows
Open VS Code and select Terminal.
Make sure the terminal is using PowerShell or Command Prompt.

Run:
```bash
ssh -m hmac-sha2-256 qnxuser@<Raspberry_Pi_IP>
```
For example, if your Raspberry Pi IP is 10.37.87.53. Run
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

## Linux
Open VS Code and select Terminal.

Run:
```bash
ssh -m hmac-sha2-256 qnxuser@<Raspberry_Pi_IP>
```
For example, if your Raspberry Pi IP is 10.37.87.53. Run
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


