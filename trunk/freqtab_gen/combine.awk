{
	cmd="grep " $3 " data/freqtable.gbkonly.txt";
	cmd | getline freqline;
	freq = 0;
	close(cmd);
	if (freqline != "") {
		split(freqline, seg, "\t");
		freq = seg[2];
	}
	print $0, freq;
}
