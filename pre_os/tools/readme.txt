this tool:
1. is used to append other binaries (e.g. starter.bin, xmon_loader, startap, xmon) to ikgt_pkg.bin
2. after that it will update the file offset header in ikgt_pkg.bin file.
3. also, it does build time oversize check, to find error as early as possible.
4. will pack secondary guest image if it exists in pre_os/build/linux/release


usage:
  [<OPTION> <FILE>] ...
options:
  --starter  specify the name of starter file. if no this option, default is starter.bin
  --xmon_loader    specify the name of xmon_loader file. if no this option, default is xmon_loader.bin
  --startap  specify the name of startap file. if no this option, default is startap.bin
  --xmon     specify the name of xmon.bin file. if no this option, default is xmon.bin
  --sguest   specify the name of secondary guest image file. if no this option, default is lk.bin




  --- end of file ---
