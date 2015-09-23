set -e

MAKE_CMD="make -f Makefile"

rm -rf out/

mkdir out
# Generate prebuilts for x86_64
$MAKE_CMD ARCH=x86_64
cp preload.efi ./out/preload_x64.efi
$MAKE_CMD ARCH=x86_64 clean
