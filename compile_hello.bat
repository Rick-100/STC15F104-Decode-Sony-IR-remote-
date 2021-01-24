
sdcc -mmcs51  --iram-size 128 --xram-size 0 --code-size 4088 --verbose Sony_no_int.c -o Sony_no_int.ihx
packihx Sony_no_int.ihx > Sony_no_int.hex

pause
