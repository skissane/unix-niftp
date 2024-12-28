#!/bin/awk -f
/^#/	{ next }
(NF < 2) { next }
{	print "h:uk.ac.cambridge.computer-lab."$2":uk.ac.cam.cl."$2":ether:3:"$1":911:"$1":911::50:1:"
	print "h:uk.ac.cambridge.computer-lab."$2":uk.ac.cam.cl."$2":ether:4:"$1":911:"$1":911::::"
	if (NF > 2) {
		print "h:uk.ac.cambridge.computer-lab."$3":uk.ac.cam.cl."$3":ether:3:"$1":911::::50:1:"
		print "h:uk.ac.cambridge.computer-lab."$3":uk.ac.cam.cl."$3":ether:4:"$1":911::::::"
	}
}
