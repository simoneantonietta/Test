# script file
#
# fw force <device>
# to force the upload of a firmware regardless the version installed
# <device> can be "varcolan";"supply";
#
# exe <script> <param1> <param2> ...
# perform execution of abash script in the current folder with its parameters
#

#fw force varcolan

# BADGE
# syntax:
#       --------------------  vi st pin    vstart vstop     prof1 prof2 etc
# badge 1234567890abcde      (0, 0, 00000, now,   tomorrow, 0, ............)
# vstart can be "now" or YYYY-MM-DD
# vstop can be "tomorrow" or YYYY-MM-DD
# profileID the first must be declared, the others are optional

#badge 1234567890abcde 	(0, 2, 00000, now,   tomorrow, 0)
#badge 332223313131233 	(0, 2, 12345, now,   tomorrow, 0)
#badge 112234433454453 	(0, 2, 54321, now,   tomorrow, 0)
#badge 057486834ade530 	(0, 2, 11111, now,   tomorrow, 0)
#badge 101010101010101	(0, 2, 11111, now,   tomorrow, 0)
#badge 10100000000000	(0, 2, 11111, now,   tomorrow, 0)
#badge 1010004863793	(0, 2, 22222, now,   tomorrow, 0)

# other available commands

#clean all badge
#disable badgeautosend
#reload profiles|weektimes|terminals
#profile 0 active|inactive 0,20,50
# you can use enable|disable too
#weektime 0 mon|tue|wed|thu|fri|sat|sun 10:00,12:00 14:00,17:00
# you can use day in italian too
#logout
#set 3 0 2