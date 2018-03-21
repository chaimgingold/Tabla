echo "<group>
loop_mode=one_shot"

DIR=drums
DRUMS=`ls $DIR`

NOTE=72
for DRUMFILE in $DRUMS; do
echo "
<region>
sample=$DIR/$DRUMFILE
lokey=$NOTE
hikey=$NOTE
lovel=1
hivel=127
pitch_keycenter=$NOTE
"
NOTE=$(($NOTE + 1))
done
