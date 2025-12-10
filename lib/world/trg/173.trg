#17300
Garimpando ouro~
2 c 100
p~
* Não deixe alguém fazer spam no gatilho para ganhar dinheiro. Tire alguns pontos de movimento a cada rodada e pare quando chegarem a 10
if %actor.move% <= 10
  %send% %actor% Você está exausto demais para continuar.
  halt
end
* Dispara em 'garimpar ouro' ou abreviações de cada palavra
if %cmd% /= garimpar && %arg% /= ouro
  eval heldobj %actor.eq(hold)%
  * Certifique-se de que pegaram a bacia de ouro na sala 17308 e a estejam segurando
  if %heldobj.vnum% == 17350
    %send% %actor% Você mergulha sua bacia em um barril de água e retira um pouco de terra e começa a girá-la na água.
    %echoaround% %actor% %actor.name% mergulha uma bacia em um barril de água e começa a garimpar ouro.
    * Tire 10 pontos de movimento, espere 3 segundos e dê uma chance de 1 em 10 de sucesso.
    nop %actor.move(-10)%
    wait 3 sec
    if %random.10% == 1
      %send% %actor% Você encontra uma pequena pepita de ouro no fundo de sua bacia.
      %echoaround% %actor% %actor.name% pega algo de %actor.hisher% bacia.
      * Dê a eles uma pepita
      %load% obj 17351 %actor% inv
    else
      %send% %actor% Você não encontra nada de valor.
    end
  else
    %send% %actor% Você precisa de uma bacia para isso.
  end
end
~
$~
