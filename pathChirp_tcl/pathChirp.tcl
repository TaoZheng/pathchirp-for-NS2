#Create a simulator object
set ns [new Simulator]

#Open a trace file

$ns trace-all [open all.out w]


#Define a 'finish' procedure
proc finish {} {
        global ns 
        $ns flush-trace

        exit 0
}

#Create three nodes
set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]

#Connect the nodes with two links
$ns duplex-link $n0 $n1 1000Mb 10ms DropTail
$ns duplex-link $n3 $n1 1000Mb 10ms DropTail
$ns duplex-link $n1 $n2 10Mb 10ms DropTail

#Create two ping agents and attach them to the nodes n0 and n2
set p0 [new Agent/pathChirpSnd]
# Note: the following values need not be set, pathChirp in general 
# will adapt its probing rates according to the situation

$p0 set lowrate_ 1000000.0   # lowest rate in chirp train
$p0 set highrate_ 15000000.0 # highest rate in chirp train
$p0 set avgrate_ 300000.0    # average data probing rate
$p0 set packetSize_ 1200  # probe packet size
$p0 set spreadfactor_ 1.2  # spread factor within chirp

$ns attach-agent $n0 $p0

set p1 [new Agent/pathChirpRcv]
# Note: the following values need not be set, pathChirp in general 
# will adapt its probing rates according to the situation

$p1 set packetSize_ 64  # feedback packet size
$p1 set num_inst_bw 7   # number of chirp estimates to smooth over
$p1 set decrease_factor 3 # excursion detection parameter
$p1 set busy_period_thresh 5 # excursion detection parameter





$ns attach-agent $n2 $p1

#Connect the two agents
$ns connect $p0 $p1

# CBR CROSS TRAFFIC

set cbr [new Application/Traffic/CBR]
$cbr set packetSize_ 1000
$cbr set rate_ 5Mb #to get 1 percent of bandwidth

set udp [new Agent/UDP]
$ns attach-agent $n3 $udp
$cbr attach-agent $udp
set null0 [new Agent/Null] 
$ns attach-agent $n2 $null0 
$ns connect $udp $null0

#Schedule events
#Note: must start both the pathChirp sender and receiver
$ns at 0.2 "$p0 start"
$ns at 0.2 "$p1 start"
$ns at 150.0 "$p0 stop"


$ns at 50 "$cbr start"
$ns at 100.0 "$cbr stop"

$ns at 160.0 "finish"

#Run the simulation
$ns run



