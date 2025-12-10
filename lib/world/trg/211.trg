#21100
Test~
0 g 100
~
%echo% Esta lista de comandos do gatilho não está completa!
~
#21101
Carregar Cartas~
0 d 100
*~
set zone 211
set start %self.room.vnum%
eval room %self.room.vnum% + 1
switch %speech.car%
  case shuffle
    if %self.varexists(Cards_Dealt)%
      %echo% @n  A voz está em sua mente novamente.
      %echo% @c    'Desculpe, as cartas parecem já estar dispostas.'@n
      halt
    else
      set deck 78
      set layout 10
      global deck
      global layout
      set var %zone%01
      emote embaralha as cartas.
      %echo% @n  %self.name% parece falar diretamente com sua mente.
      %echo% @c    'Continue embaralhando até sentir que o baralho entende sua pergunta.@n
      %echo% @c   Quando estiver pronto, diga DEAL.'@n
      set Deck_Shuffled 1
      global Deck_Shuffled
      while %var% < %zone%79
        set %var% 1
        remote %var% %self.id%
        eval var %var% + 1
      done
      halt
    end
  end
  case deal
  if !%self.varexists(Deck_Shuffled)%
    %echo% @n  A voz está em sua mente novamente.
    %echo% @c    'As cartas não parecem entender sua pergunta ainda. Você @n
    %echo% @c   EMBARALHOU?'@n
    halt
  elseif %self.varexists(Cards_Dealt)%
    %echo% @n  A voz está em sua mente novamente.@n
    %echo% @c    'Desculpe, as cartas parecem já estar dispostas.'@n
    halt
  else
    emote começa a distribuir as cartas.
    %echo% @n  A voz parece envolvê-lo agora.
    %echo% @c    'Quando estiver pronto, por favor vá para cima para começar sua leitura.  Uma vez que você@n
    %echo% @c   comece, você não poderá voltar.  Claro, você sempre pode@n
    %echo% @c   voltar para outra leitura.@n
    wait 2 sec
    %echo% @c    Em cada sala, LOOK CARD para ver o significado.  Reverso significa@n
    %echo% @c   que a carta foi disposta de cabeça para baixo, o que muda o significado.@n
    %echo% @c   Não se preocupe com isso.  A carta mostrará o significado reverso.@n
    %echo% @c   O nome da sala explicará o que a colocação da carta significa.'@n
    wait 1 sec
    %door% %self.room.vnum% up flags a
    emote abre a porta para a escadaria.
    while %layout%
      set zonebase %zone%00
      eval card %random.78% + %zonebase%
      eval temp %%self.varexists(%card%)%%
      eval hascard %temp%
      if %hascard%
        mgoto %room%
        set rand %random.2%
        if %rand% == 1
          %load% obj %zone%99
        end
        %load% obj %card%
        mgoto %start%
        rdelete %card% %self.id%
        eval deck %deck% - 1
        eval layout %layout% - 1
        eval room %room% + 1
        global deck
        global layout
        set Cards_Dealt 1
        global Cards_Dealt
      else
      end
    done
    halt
  break
  default
  break
end
~
#21102
Olhar Carta~
2 c 100
*~
* Parnassus' Special Anti-Freeze Formula
if %cmd.mudcommand% == nohassle
  return 0
  halt
end
*
set zone 211
if %self.vnum% > %zone%10 && %self.vnum% < %zone%24
  * if %self.vnum% > %zone%00 && %self.vnum% < %zone%24
  set cmdroom %zone%20
elseif %self.vnum% > %zone%30 && %self.vnum% < %zone%44
  set cmdroom %zone%40
elseif %self.vnum% > %zone%45 && %self.vnum% < %zone%64
  set cmdroom %zone%60
else
  return 0
  halt
end
if %cmd.mudcommand% == look || %cmd.mudcommand% == examine
  * Look Trigger Written by Fizban - June 06 2013
  * This trigger changes the meaning of the card for reverse.
  * If there is no argument, just look.
  if !%arg%
    return 0
    halt
  else
    * Check for the reverser.  If it is in the room, give
    *  one meaning.  If it is not, give the other.
    eval rev %%findobj.%self.vnum%(%zone%99)%%
    * The ~ anchors the comparison to the front of the word.
    * rd /= card but ~rd is not a part of ~card while ~c is.
    set arg ~%arg%
    if ~reverse /= %arg%
      if %rev% < 1
        %send% %actor% Você não vê isso aqui.
        halt
      else
        %send% %actor% A carta está de cabeça para baixo.
        halt
      end
    end
    if ~carta /= %arg%
      if %rev% < 1
        %force% %actor% look carta
      else
        %force% %actor% look reverso
      end
    else
      return 0
    end
  end
elseif %cmd.mudcommand% == quit || %cmd.mudcommand% == afk
  %send% %actor% Porque você decidiu %cmd.mudcommand%, você não pode terminar a leitura.
  %echoaround% %actor% %actor.name% tem que deixar a leitura agora.
  wait 1 sec
  %send% %actor% Você é magicamente enviado ao final da leitura.
  %echoaround% %actor% %actor.name% é levado embora em uma nuvem de fumaça.
  wait 1 sec
  %teleport% %actor% %cmdroom%
  wait 1 sec
  %at% %cmdroom% %force% %actor% down
  wait 1 sec
  %at% %actor% %force% %actor% %cmd% %arg%
  wait 1 sec
  halt
elseif %cmd% == return || %cmd% == recall || %cmd% == teleport || %cmd.mudcommand% == goto
  %send% %actor% Porque você decidiu %cmd%, você não pode terminar a leitura.
  %echoaround% %actor% %actor.name% tem que deixar a leitura agora.
  wait 1 sec
  %send% %actor% Você é magicamente enviado ao final da leitura.
  %echoaround% %actor% %actor.name% é levado embora em uma nuvem de fumaça.
  wait 1 sec
  %teleport% %actor% %cmdroom%
  wait 1 sec
  %at% %cmdroom% %force% %actor% down
  wait 1 sec
  %at% %actor% %force% %actor% %cmd% %arg%
  wait 1 sec
  halt
else
  return 0
end
~
#21103
Limpar as Cartas - r21120, r21140, r21160~
2 q 100
~
* Clears cards from the reading and reader marker
* when player finishes the reading.
if %direction% == down
  wait 2 sec
  %purge%
  set room %self.vnum%
  eval purgeroom %room% - 10
  %door% %purgeroom% up flags abcd
  eval purgeroom %purgeroom% + 1
  while %purgeroom% < %self.vnum%
    %at% %purgeroom% %purge%
    eval purgeroom %purgeroom% + 1
  done
  eval purgeroom %purgeroom% + 5
  %at% %purgeroom% %purge%
end
~
#21104
Reiniciar o Leitor de Fortuna~
0 q 100
~
* Clears the fortune-teller for the next player.
* Closes the doors coming back to make it easier to see the path while
*   keeping the possibility of checking past cards.
if %direction% == up
  * set deck 78
  set layout 10
  * global deck
  global layout
  rdelete Deck_Shuffled %self.id%
  rdelete Cards_Dealt %self.id%
  rdelete tarot_reading_started %self.id%
  if %self.room.vnum% == 21110 || %self.room.vnum% == 21130 || %self.room.vnum% == 21150
    * %door% %self.room.vnum% up flags abc
    eval cardroom %self.room.vnum% + 2
    %door% %cardroom% down flags ab
    eval cardroom %cardroom% + 1
    %door% %cardroom% north flags ab
    eval cardroom %cardroom% + 1
    %door% %cardroom% south flags ab
    eval cardroom %cardroom% + 1
    %door% %cardroom% west flags ab
    eval cardroom %cardroom% + 1
    %door% %cardroom% north flags ab
    eval cardroom %cardroom% + 1
    %door% %cardroom% west flags ab
    eval cardroom %cardroom% + 1
    %door% %cardroom% south flags ab
    eval cardroom %cardroom% + 1
    %door% %cardroom% south flags ab
    eval cardroom %cardroom% + 1
    %door% %cardroom% south flags ab
    eval cardroom %cardroom% + 1
    %door% %cardroom% east flags ab
    eval cardroom %cardroom% + 1
    %door% %cardroom% south flags ab
    eval cardroom %cardroom% + 1
    %door% %cardroom% west flags ab
    eval cardroom %cardroom% + 1
    %door% %cardroom% north flags ab
  end
end
~
#21105
Vendedor Saúda~
0 h 100
~
wait 2 sec
%echo% @n  A voz de %self.name% parece preencher sua cabeça.
%echo% @c     'Ahh, você tem algo em mente? Vamos ver o que as@n
%echo% @c   cartas têm a dizer.  Infelizmente, você não pode segurar ou embaralhar@n
%echo% @c   minhas cartas, mas concentre-se em sua pergunta e diga shuffle.@n
%echo% @c   Quando você sentir que as cartas conhecem sua pergunta, diga deal e@n
%echo% @c   eu distribuirei as cartas para você examinar.@n
wait 3 sec
%echo% @c     Normalmente eu interpretaria as cartas para você, mas isso é@n
%echo% @c   proibido a mim neste espaço e tempo.  Tudo que me é permitido é@n
%echo% @c   mostrar-lhe as cartas e você deve decidir seus significados em sua@n
%echo% @c   própria mente. Mova-se de carta em carta.  Cada espaço e cada carta@n
%echo% @c   explicará a si mesma para você. 'LOOK CARTA' em cada sala para ver@n
%echo% @c   a explicação. Estes são significados muito simplificados, então eles@n
%echo% @c   são muito gerais.@n
wait 3 sec
%echo% @c     Lembre-se, isto é apenas um jogo e não deve ser levado@n
%echo% @c   a sério mais do que você viveria sua vida pelos@n
%echo% @c   horóscopos de jornal ou mensagens de biscoitos da sorte.@n
wait 2 sec
%echo% @c     Quando estiver pronto, comece dizendo SHUFFLE.'@n
~
#21106
Recepcionista faz malabarismos com agendamentos - M21104~
0 d 100
*~
set zone 211
* set findobj 0
* Checks for available readers
* Kicks out people that are afk, etc
if %self.room.vnum% != %zone%02
  emote looks around in confusion.
  say Desculpe. Preciso ir ao meu escritório.
  emote leaves.
  eval findmob %%findmob.%zone%02(%self.vnum%)%%
  if %findmob% > 0
    %purge% %self%
  else
    mgoto %zone%02
    halt
  end
end
if %actor% == %self%
  halt
end
* This loop goes through the entire string of words the actor says. .car is the
* word and .cdr is the remaining string.
eval word %speech.car%
eval rest %speech.cdr%
while %word%
  *   %echo% Word: %word%
  *   %echo% rest: %rest%
  switch %word%
    * Appointment starts the conversation.
    * Objxxx98 keeps trigger from reacting to other conversations.
    * if %actor.is_pc% &&
    case appointment
    * Check to see if someone is already trying to get an appointment.
    if %self.has_item(%zone%98)% && !%actor.varexists(Making_Tarot_Appointment_%zone%)%
      say Desculpe, %actor.name%. Estou falando com outra pessoa agora.
      halt
    end
    if !%self.has_item(%zone%98)%
      %load% o %zone%98
      set Making_Tarot_Appointment_%zone% 1
      remote Making_Tarot_Appointment_%zone% %actor.id%
      say Deixe-me ver se algum de nossos consultores tem uma vaga.
      say Diga restart a qualquer momento para sair ou começar de novo.
      %echo% Ela consulta uma agenda.
      wait 2 seconds
      set available 0
      set readerno 0
      set unreaderno 0
      eval temp %%findobj.%zone%25(card)%%
      eval findobjsibyl %temp%
      if %findobjsibyl% < 1
        eval available %available% + 1
        eval readerno %readerno% + 1
        set reader%readerno% Sibyl
      else
        eval unreaderno %unreaderno% + 1
        set unreader%unreaderno% Sibyl
      end
      eval temp %%findobj.%zone%45(card)%%
      eval findobjesmerelda %temp%
      if %findobjesmerelda% < 1
        eval available %available% + 1
        eval readerno %readerno% + 1
        set reader%readerno% Esmerelda
      else
        eval unreaderno %unreaderno% + 1
        set unreader%unreaderno% Esmerelda
      end
      eval temp %%findobj.%zone%65(card)%%
      eval findobjjaelle %temp%
      if %findobjjaelle% < 1
        eval available %available% + 1
        eval readerno %readerno% + 1
        set reader%readerno% Jaelle
      else
        eval unreaderno %unreaderno% + 1
        set unreader%unreaderno% Jaelle
      end
      if %available% == 0
        say Desculpe, Sibyl, Esmerelda e Jaelle parecem estar com clientes agora.
        say Por favor, tente novamente mais tarde.
        rdelete Making_Tarot_Appointment_%zone% %actor.id%
        halt
      end
      if %available% == 1
        say %unreader1% e %unreader2% estão com clientes, mas %reader1% está disponível.
        say Diga %reader1% se você quiser vê-la.
        set Choosing_Tarot_Reader_%zone% 1
        remote Choosing_Tarot_Reader_%zone% %actor.id%
        halt
      end
      if %available% == 2
        say %unreader1% está com um cliente, mas %reader1% e %reader2% estão disponíveis.
        say Diga %reader1% ou %reader2% para vê-la.
        set Choosing_Tarot_Reader_%zone% 1
        remote Choosing_Tarot_Reader_%zone% %actor.id%
        halt
      end
      if %available% == 3
        say %reader1%, %reader2% e %reader3% estão todos disponíveis agora.
        say Diga %reader1%, %reader2% ou %reader3% para vê-la.
        set Choosing_Tarot_Reader_%zone% 1
        remote Choosing_Tarot_Reader_%zone% %actor.id%
        halt
      end
    end
  break
  case Sibyl
    if %actor.varexists(Choosing_Tarot_Reader_%zone%)%
      eval findobj %%findobj.%zone%25(card)%%
      if %findobj% < 1
        say Sibyl está pronta para vê-lo agora.
        %door% %zone%02 north flags a
        wait 1 sec
        %force% %actor% north
        %door% %zone%02 north flags abc
        rdelete Making_Tarot_Appointment_%zone% %actor.id%
        rdelete Choosing_Tarot_Reader_%zone% %actor.id%
        mgoto %zone%99
        %purge% quill
        mgoto %zone%25
        %load% obj %zone%49
        mgoto %zone%02
      else
        say Desculpe. Sibyl está com outro cliente agora.
        say Por favor, escolha um dos leitores disponíveis.
      end
    end
  break
  case Esmerelda
    if %actor.varexists(Choosing_Tarot_Reader_%zone%)%
      eval findobj %%findobj.%zone%45(card)%%
      if %findobj% < 1
        say Esmerelda está pronta para vê-lo agora.
        %door% %zone%02 west flags a
        wait 1 sec
        %force% %actor% w
        %door% %zone%02 west flags abc
        rdelete Making_Tarot_Appointment_%zone% %actor.id%
        rdelete Choosing_Tarot_Reader_%zone% %actor.id%
        mgoto %zone%99
        %purge% quill
        mgoto %zone%45
        %load% obj %zone%52
        mgoto %zone%02
      else
        say Desculpe. Esmerelda está com outro cliente agora.
        say Por favor, escolha um dos leitores disponíveis.
      end
    end
  break
  case Jaelle
    if %actor.varexists(Choosing_Tarot_Reader_%zone%)%
      eval findobj %%findobj.%zone%65(card)%%
      if %findobj% < 1
        say Jaelle está pronta para vê-lo agora.
        %door% %zone%02 east flags a
        wait 1 sec
        %force% %actor% e
        %door% %zone%02 east flags abc
        rdelete Making_Tarot_Appointment_%zone% %actor.id%
        rdelete Choosing_Tarot_Reader_%zone% %actor.id%
        mgoto %zone%99
        %purge% quill
        mgoto %zone%65
        %load% obj %zone%50
        mgoto %zone%02
      else
        say Desculpe. Jaelle está com outro cliente agora.
        say Por favor, escolha um dos leitores disponíveis.
      end
    end
  break
  case Restart
    if %actor.varexists(Making_Tarot_Appointment_%zone%)%
      rdelete Making_Tarot_Appointment_%zone% %actor.id%
      rdelete Choosing_Tarot_Reader_%zone% %actor.id%
      mgoto %zone%99
      %purge% quill
      mgoto %zone%02
      emote coloca o livro de agendamentos de lado.
    end
  break
  default
  break
done
* End of the loop we need to take the next word in the string
* and save the remainder for the next pass.
eval word %rest.car%
eval rest %rest.cdr%
done
~
#21107
Tarot Receptionist greets - M21104~
0 h 100
*~
if %direction% == south
  welcome %actor.name%
  %send% %actor% Ana says, 'Você gostaria de fazer um agendamento com um de nossos leitores?'
  %send% %actor% Ana says, 'Antes de começarmos, certifique-se de ter tempo suficiente para terminar sua leitura.'
  %send% %actor% Ana says, 'Por favor, não fique away ou saia do jogo antes de terminar a leitura.'
  %send% %actor% Ana says, 'Se você tiver certeza, apenas diga appointment.'
elseif %direction% == up
  smile %actor.name%
  %send% %actor% Ana says, 'Espero que tenha gostado da sua leitura. Por favor, volte em breve.'
  %send% %actor% Ana says, 'Claro, se você quiser outro agendamento agora, diga appointment.'
end
~
#21108
Saindo do Tarô~
0 c 100
*~
* For mobs to clear reading from players blocking by starting and leaving.
* Should be adjusted to your muds commands.
* Parnassus' Special Anti-Freeze Formula
if %cmd.mudcommand% == nohassle
  return 0
  halt
end
*
set zone 211
if %cmd.mudcommand% == quit || %cmd.mudcommand% == afk
  if %self.vnum% == %zone%04 && %actor.varexists(Making_Tarot_Appointment_%zone%)%
    say Desculpe, mas não poderei dar-lhe um agendamento agora.
    say Por favor, volte quando tiver mais tempo disponível.
    rdelete Making_Tarot_Appointment_%zone% %actor.id%
    rdelete Choosing_Tarot_Reader_%zone% %actor.id%
    mgoto %zone%99
    %purge% quill
    mgoto %zone%02
    wait 1 sec
    emote coloca o livro de agendamentos de lado.
    wait 1 sec
    %force% %actor% %cmd.mudcommand%
    wait 1 sec
    halt
  elseif %self.vnum% == %zone%01 || %self.vnum% == %zone%02 || %self.vnum% == %zone%03
    set office %self.room.vnum%
    eval endroom %office% + 10
    %echo% @n    %self.name%', sua voz soa reprovadora em sua cabeça.
    %echo% @c       'Você não parece ter tempo para isso agora.@n
    %echo% @c     Por favor, volte quando tiver mais tempo.'@n
    wait 1 sec
    %echo%    %self.name% acena com a mão e você se encontra do lado de fora.
    wait 1 sec
    %teleport% %actor% %zone%01
    mgoto %endroom%
    down
    mgoto %office%
    wait 1 sec
    %force% %actor% look
    %force% %actor% %cmd.mudcommand%
    rdelete Deck_Shuffled %self.id%
    rdelete Cards_Dealt %self.id%
    rdelete tarot_reading_started %self.id%
    halt
  else
    return 0
    halt
  end
elseif %cmd% == return || %cmd% == recall || %cmd% == teleport || %cmd.mudcommand% == goto
  if %self.vnum% == %zone%04 && %actor.varexists(Making_Tarot_Appointment_%zone%)%
    say Desculpe, mas não poderei dar-lhe um agendamento agora.
    say Por favor, volte quando tiver mais tempo disponível.
    rdelete Making_Tarot_Appointment_%zone% %actor.id%
    rdelete Choosing_Tarot_Reader_%zone% %actor.id%
    mgoto %zone%99
    %purge% quill
    mgoto %zone%02
    %send% %actor%  @n
    emote coloca o livro de agendamentos de lado.
    %send% %actor%  @n
    return 0
    halt
  elseif %self.vnum% == %zone%01 || %self.vnum% == %zone%02 || %self.vnum% == %zone%03
    set office %self.room.vnum%
    eval endroom %office% + 10
    %echo% @n    %self.name%', sua voz soa reprovadora em sua cabeça.
    %echo% @c       'Você não parece ter tempo para isso agora.@n
    %echo% @c     Por favor, volte quando tiver mais tempo.'@n
    %send% %actor%  @n
    %teleport% %actor% %zone%01
    return 0
    %send% %actor%  @n
    mgoto %endroom%
    down
    mgoto %office%
    rdelete Deck_Shuffled %self.id%
    rdelete Cards_Dealt %self.id%
    rdelete tarot_reading_started %self.id%
    halt
  else
    return 0
    halt
  end
elseif %cmd.mudcommand% == south
  if %self.vnum% == %zone%04 && %actor.varexists(Making_Tarot_Appointment_%zone%)%
    say Desculpe, mas não poderei dar-lhe um agendamento agora.
    say Por favor, volte quando tiver mais tempo disponível.
    rdelete Making_Tarot_Appointment_%zone% %actor.id%
    rdelete Choosing_Tarot_Reader_%zone% %actor.id%
    mgoto %zone%99
    %purge% quill
    mgoto %zone%02
    wait 1 sec
    emote coloca o livro de agendamentos de lado.
    wait 1 sec
    %force% %actor% %cmd%
    halt
  else
    return 0
    halt
  end
else
  return 0
end
~
#21109
Timer for obj 21198~
1 f 100
~
* Timer on obj 21198 is set to 10 minutes.  This is adjustable.
* Since 21198 stops any appointments while talking to one person
*  this keeps any person from blocking the zone until reboot.
set zone 211
set actor %self.carried_by%
if %actor.vnum% == %zone%04
  %echo% %actor.name% says, 'I've been waiting too long for this appointment.'
  %echo% %actor.name% coloca o livro de agendamentos de lado.
  %purge% %self%
else
  set actor %self.carried_by.name%
  %force% %actor% say I seem to have stolen someone's pen.
  %echoaround% %actor% The nib of the pen pokes %actor%.
  %send% %actor% The nib of the pen pokes you.
  %damage% %actor% 5
  %echoaround% %actor% %actor.name% shakes %actor.hisher% hand in pain and drops a pen which rolls away.
  %send% %actor% You drop a pen which rolls away somewhere.
  %purge% %self%
end
~
#21110
Reload glass and bread - obj 21180 and 21182~
1 c 100
*~
* This trigger is to keep the waiting room supplied with food and drink.
* Because of the auto-regenerative qualities, it also cancels out any
*  sac benefits to prevent spam-saccing for gold or exp.
* Parnassus' Special Anti-Freeze Formula
if %cmd.mudcommand% == nohassle
  return 0
  halt
end
*
set zone 211
if get == %cmd.mudcommand% || sacrifice == %cmd.mudcommand%
  if %self.room.vnum% == %zone%02
    set testernumber 2
  else
    set testernumber 1
  end
  set arg _%arg%
  eval inroom %self.room%
  eval obj %inroom.contents%
  * find the first object in the room
  while %obj%
    set next_obj %obj.next_in_list%
    set objlist %obj.name%
    set keywordlist _%obj.name.car%
    set keywordrest _%obj.name.cdr%
    while %keywordlist%
      * while an object is in the room
      if %keywordlist.contains(%arg%)%
        if %obj.id% == %self.id%
          if get == %cmd.mudcommand%
            %force% %actor% %cmd.mudcommand% %obj.name.car%
          elseif sacrifice == %cmd.mudcommand%
            %send% %actor% You carefully dispose of %obj.shortdesc%.
            %echoaround% %actor% %actor.name% carefully disposes of %obj.shortdesc%.
            set me %self.vnum%
            eval temp %%findobj.%zone%02(%me%)%%
            eval tester %temp%
            if %self.room.vnum% == %zone%02
              set testernumber 2
            else
              set testernumber 1
            end
            if %tester% < %testernumber%
              %at% %zone%02 %load% obj %self.vnum%
            end
            %purge% %self%
          end
          set me %self.vnum%
          eval temp %%findobj.%zone%02(%me%)%%
          eval tester %temp%
          if %tester% < %testernumber%
            %at% %zone%02 %load% obj %self.vnum%
          end
          halt
        end
      end
      set keywordlist %keywordrest.car%
      set keywordrest %keywordrest.cdr%
    done
    * find the next object for the while to loop
    set obj %next_obj%
  done
  return 0
  halt
else
  return 0
  halt
end
~
$~
