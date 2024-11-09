The presented files are based on the original code published by Triften Chmil on Github about 10 years ago.
I have modified the code after its analysis and comparison with my snoop logs done at different resolutions.
Now there are 3 groups of code + consts files for scanning at 600, 300 and 150 dpi resolutions.
As the scan results depend much on the calibration data I have added a calibration procedure.

As before, the compiling consist of

    gcc scan_150.c -o scan -lusb        for scanning (at 150 dpi resolution)
    
    gcc -o cfix_f cfix_f.c              (for other code).

Some of the modifications that deserve a mention consist of:
- reduction of the code (parts considered to be of no use were removed)
- change of interaction with the user (when a name already used is given as input the user will be asked for a new one)
- as the time spent by warming up the lamp takes most of the scanning time the relevant part is used only once for repeated scans
- no separate fix code is needed (if necessary it is incorporated in the scan code using tempotary files).

Known issues:
- the quality of the scans depends heavily on the condition of the scanner at the moment
  . if the scanner is powered on just before scanning the lamp stays on even after scanning is ended
    this is not a normal condition, after some time the lamp will go off and the scanner goes into stand-by
    only then the scan results will have "standard" quality.
    before this condition is reached the scans will be lighter with shifted color rendition
  . if the scanner is left for a long time waiting for user input (to scan more)
    probably the scan result will be lighter, just check it and if needed repeat the scan.
- if the sample contains large areas of uniform (usually dark) color
  in the scan can be visible differences in lighting between neighboring strips
  if there are parallel lines in the sample try to orient them in the direction of the head movement
  (it is strange but the orientation makes then much difference).
- there is a strong visible difference in lighting between scans at 600 dpi and 300 or 150 dpi
  the scans at lower resolutions seem darker with enhanced colours
  as the calibration procedure must be done at 600 dpi resolution you can
  . use the cfix_f application (see the description in Details.txt)
  . take the results as they are (depending on your prefered resolution)
  . calibrate for lighter result at 600 dpi and check the impact for 150 dpi
  . use modified SetRegisters2ax values for one or the other resolution
  (scanning at higher resolution and then converting is only useful for getting lighter scans
  it comes at the price of lower speed of scanning and slightly different resulting dimensions).
- scanning at 600 dpi is rather slow and writes about 120 MB to the disk
  avoid any activity accessing the disk at this time, it makes the scanner rescan part of the picture so it becomes broken

Note:
The standard 150 dpi scan uses maximum width of the scanned image.
Because of that it cannot meet the BMP rule of division by four, 2 extra bytes must be added at the end of each line.
The code that writes the picture data to the output file takes care of that.
Also the code for calculating the RGB values does that.

More information on the code, its modification, calibration and more options is included in the file Details.txt

List of supplied files:
- scan_150.c + hp4600consts_150.h
- scan_153.c + hp4600consts_300.h (scanning at 300 and conversion to 150 dpi)
- scan_156.c + hp4600consts_600.h (scanning at 600 and conversion to 150 dpi)
- scan_300.c + hp4600consts_300.h
- scan_600.c + hp4600consts_600.h
- scan_152.c + hp4600consts_150.h (example of SetRegisters2ax enhanced by 2)
- scan_cal.c + hp4600consts_600.h (scanning for calibration)
- calibration.ods (calc sheet used for supplied consts files)
- optional color fixing files (cfix et all) will be enumerated in Details.txt
