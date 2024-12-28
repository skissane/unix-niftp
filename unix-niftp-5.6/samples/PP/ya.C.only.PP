# /etc/ybts-auth for pb's ybtsd (part of x25d)
#
# Complex example:
# ----------------
# calls for FTP, FTP.PP, FTP.MAIL and FTP.NEWS are directed to different
# programs (all symbolic links to the same object), so as to keep log
# separate, allow fine control, etc.
#
# Also separate listeners for 0000 (janet), 2342 (PSS) and the rest (IPSS).
/usr/lib/niftp/ljf:ftp:0000*
/usr/lib/niftp/ljp:ftp.pp:0000*
/usr/lib/niftp/ljm:ftp.mail:0000*
/usr/lib/niftp/ljn:ftp.news:0000*
/usr/lib/niftp/lpf:ftp:2342*
/usr/lib/niftp/lpp:ftp.pp:2342*
/usr/lib/niftp/lpm:ftp.mail:2342*
/usr/lib/niftp/lpn:ftp.news:2342*
/usr/lib/niftp/lif:ftp:*
/usr/lib/niftp/lip:ftp.pp:*
/usr/lib/niftp/lim:ftp.mail:*
/usr/lib/niftp/lin:ftp.news:*
