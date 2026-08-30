echo "This terminal supports: $(tput colors) colors"

echo "=== Standard (0-7) ==="
for i in {0..7}; do
    tput setaf "$i"; printf "Color %3d  " "$i"
done
tput sgr0; echo

echo "=== Bright (8-15) ==="
for i in {8..15}; do
    tput setaf "$i"; printf "Color %3d  " "$i"
done
tput sgr0; echo

echo "=== 216 color cube (16-231) ==="
for i in {16..231}; do
    tput setaf "$i"; printf "%3d " "$i"
    tput sgr0
    (( (i - 15) % 12 == 0 )) && echo
done
echo

echo "=== Grayscale (232-255) ==="
for i in {232..255}; do
    tput setaf "$i"; printf "%3d " "$i"
done
tput sgr0; echo
