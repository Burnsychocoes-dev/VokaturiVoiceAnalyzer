/*
* SharedBuffer.c
* This software is released under the GNU General Public License. No warranty.
*
* version 2016-04-17
*/
#include "stdafx.h"
#include "SharedBuffer.h"



/*
The definition of the global struct via which the recording thread communicates
with the GUI thread.
*/
struct SharedBuffer theSharedBuffer;

/* End of file SharedBuffer.c */