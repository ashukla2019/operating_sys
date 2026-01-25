

# Linux Commands Cheat Sheet (One Page with Examples)

---

## 1️⃣ Login & System Information
```bash
whoami            # current user
uname -a          # OS & kernel info
uptime            # system running time
hostname          # system name
who               # logged-in users


🐧 Linux Commands Cheat Sheet (One Page with Examples)
1️⃣ Login & System Information
whoami            # current user
uname -a          # OS & kernel info
uptime            # system running time
hostname          # system name
who               # logged-in users
2️⃣ User & Group Management
id                # UID, GID info
groups            # groups of user
su user           # switch user
sudo command      # run command as root
passwd user       # change password
3️⃣ Directory Commands
pwd               # current directory
ls -l             # list files
cd /var/log       # change directory
mkdir test        # create directory
rmdir test        # remove empty directory
tree              # directory structure
4️⃣ File Commands
touch file.txt    # create file
cat file.txt      # view file
less file.txt     # paginated view
cp a.txt b.txt    # copy file
mv a.txt b.txt    # rename/move
rm file.txt       # delete file
file file.txt     # file type
5️⃣ Permissions & Ownership
ls -l file.txt
chmod 644 file.txt     # rw-r--r--
chmod +x script.sh    # make executable
chown user:group file.txt
umask                 # default permissions
6️⃣ Search & Text Processing
find / -name "*.log"        # find files
which python               # command path
grep "error" app.log       # search text
awk '{print $1}' file      # print column
sed 's/old/new/g' file     # replace text
cut -d: -f1 /etc/passwd    # extract field
sort file | uniq           # remove duplicates
wc -l file                 # line count
7️⃣ Pipes & Redirection
ls > out.txt           # redirect output
ls >> out.txt          # append output
cat file | grep a      # pipe
command 2> err.txt     # error redirect
ls | tee file.txt      # output + save
8️⃣ Process & Job Management
ps -ef                # process list
top                   # live monitoring
sleep 100 &           # background process
jobs                  # list jobs
fg %1                 # foreground job
kill PID              # kill process
killall nginx         # kill by name
9️⃣ Networking Commands
ip a                  # network interfaces
ping google.com       # connectivity test
ss -tuln              # open ports
curl https://site     # HTTP request
wget url              # download file
🔟 Disk & Memory Management
df -h                 # disk usage
du -sh /var/log       # directory size
free -h               # memory usage
lsblk                 # block devices
mount                 # mounted filesystems
1️⃣1️⃣ Archiving & Compression
tar -cvf files.tar dir/    # create archive
tar -xvf files.tar         # extract archive
gzip file.txt              # compress
gunzip file.txt.gz         # decompress
rsync -av src/ dest/       # sync files
1️⃣2️⃣ Shell Scripting Basics
#!/bin/bash

name="Linux"
echo $name

read user
echo "Hello $user"

if [ -f file.txt ]; then
  echo "File exists"
fi

for i in 1 2 3
do
  echo $i
done
