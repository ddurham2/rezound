

for i in jump_to_beginning.gif jump_to_next_q.gif jump_to_previous_q.gif jump_to_selection.gif pause.gif play_all_looped.gif play_all_once.gif play_selection_looped.gif play_selection_looped_gap_before_repeat.gif play_selection_looped_skip_most.gif play_selection_once.gif record.gif small_record.gif stop.gif
do
	echo $i
	convert -scale 14 $i small_$i
done


