#17600
Magik Zone 176 - Unlock East Baohe~
2 d 100
*baohe lin tsung lee-dah*~
* Magik trigger - Unlocks east exit with phrase "baohe lin tsung lee-dah"
* Original: OX magik "baohe lin tsung lee-dah" unlock east
if %speech.contains(baohe lin tsung lee-dah)%
  * Check if door exists
  if %self.east(room)%
    * Remove locked flag (keep closed flag)
    %door% %self.vnum% east flags ao
    %echo% Você escuta um ruído vindo da porta.
    * Unlock the other side if it exists
    eval otherroom %self.east(vnum)%
    %door% %otherroom% west flags ao
  end
end
~
#17601
Magik Zone 176 - Open East Baohe~
2 d 100
*baohe lin tsung lee-duh*~
* Magik trigger - Opens east exit with phrase "baohe lin tsung lee-duh"
* Original: OX magik "baohe lin tsung lee-duh" open east
if %speech.contains(baohe lin tsung lee-duh)%
  * Check if door exists
  if %self.east(room)%
    * Remove closed flag (open the door)
    %door% %self.vnum% east flags a
    %echo% A porta se abre.
    * Open the other side if it exists
    eval otherroom %self.east(vnum)%
    %at% %otherroom% %echo% A porta se abre.
    %door% %otherroom% west flags a
  end
end
~
$~
