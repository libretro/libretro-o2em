
                     O2EM v1.18 (Jan/2007)

                  an Odyssey 2 & Videopac+ emulator

      copyright 1996/1998 by Daniel Boris <dboris@comcast.net>
         
 Developed by Daniel Boris, Andre de la Rocha and Arlindo Oliveira
    
                 This package is released under the 
             Clarified Artistic License (see license.txt)
 
			 Project Homepage
                     http://o2em.sourceforge.net

---------------------------------------------------------------------------


WIP Libretro port , see original O2EM.TXT in Docs directory.

Very WIP and very bad quick hack to port O2EM to libretro.
Voice are not supported for now , sound is risky and screenshot not implemented .
At this time , works the basis but buggy on linux /android.

Remember ,Everything not working well, It's a debug release , so expect to more bug.


---------------------------------------------------------------------------


1) build

	cd build 
	make "platform=android" or make "platform=unix"
	

2) Install
	
	put bios/ and roms/ in : 

	/mnt/sdcard/O2EM/ for android

	./build           for unix 

	launch retroarch ,select game ...

3) Usage 

	L2  show/hide Statut
	R2  swap kbd pages
	L   show/hide vkbd
	R   switch Shift ON/OFF
	SEL EMU KSPACE
	STR EMU KRETURN
	A   fire/valid key in vkbd
	B   EMU KL 
	X   EMU K0
	Y   EMU K1


	VKBD :

	PAGE 1

	0	1	2	3	4	5	6	7	8	9

	+	-	*	/	=	yes 	no	clear	enter	VKBD PG2

	q	w	e	r	t	y	u	i	o	p
	
	a	s	d	f	g	h	j	k	l	SOUND OFF/ON

	z	x	c	v	b	n	m	.	?	SPACE


	PAGE 2

	0	1	2	3	4	5	6	7	8	9

	Quit	Pause	Reset	SShot	Save	Load 	Score	KBDCol	VKBD	VKBD PG1

	q	w	e	r	t	y	u	i	o	p
	
	a	s	d	f	g	h	j	k	l	SOUND OFF/ON

	z	x	c	v	b	n	m	.	?	SPACE


---------------------------------------------------------------------------

Credits:


an Odyssey 2 & Videopac+ emulator
copyright 1996/1998 by Daniel Boris <dboris@comcast.net>     
Developed by Daniel Boris, Andre de la Rocha and Arlindo Oliveira

You can download the O2EM source code here : http://o2em.sourceforge.net 


RetroArch/Libretro team : http://www.libretro.com


