#!/bin/bash

i=1;
while [ -f test$i.plan ] ; do 
	arg=`head -1 test$i.plan`
	echo $arg > /tmp/milo_test
	../milo_test $arg >> /tmp/milo_test
	ret=$?
	if [ $ret -eq 0 ] ; then
		diff /tmp/milo_test test$i.plan
		let ret=$ret+$?
	fi
	if [ $ret -eq 0 ] ; then 
		echo test $i passed
	else
		echo test $i failed
	fi
	let i=$i+1
done
