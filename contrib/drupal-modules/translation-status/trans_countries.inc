<?php
// Meaning of the columns:
// 1: 1 = pot file, 0 = po file
// 2: Flag byte used to create links on different download pages
//    Bit0 = spellchecker available (rightmost bit)
//    Bit1 = documentation available
// 3: Country code. Used to display the flag. The [flagname].png has to match this column
// 4: Full country name.
// 5: List of translators.
$countries = array(
"bg"    => array("0","01","bg","Bulgarian","Milen A. Radev"),
"bs_BA" => array("0","00","bs_BA","Bosnian/Herzegovinan","Boris Musa"),
"ca_ES" => array("0","01","ca_ES","Catalan","Arnau Perelló"),
"cs"    => array("0","01","cs","Czech","Radek Svarz, Vojtech Vondra"),
"da"    => array("0","01","da","Danish","Jan Normann Nielsen, Gunther Strube"),
"de"    => array("0","11","de","German","Stefan Küng, Lübbe Onken"),
"el"    => array("0","00","el","Greek","George Marselis"),
"eo"    => array("0","00","eo","Esperanto","Tim Morley"),
"es"    => array("0","11","es","Spanish","Fernando P. Nájera Cano, Antonio Romero Lopez"),
"fa"    => array("0","00","fa","Farsi (Persian)","Titto Agustine"),
"fr"    => array("0","11","fr","French","Olivier Booklage, Philippe Torche, Jean-Christophe Gigogne, Jérémy Badier"),
"fi"    => array("0","00","fi","Finnish","Pekka Järvinen"),
"hr"    => array("0","11","hr","Croatian","Helix d.o.o: Dinko Srkoc, Mario Saric"),
"hu"    => array("0","01","hu","Hungarian","Szente-Varga Bálint"),
"id"    => array("0","11","id","Indonesian","Zaenal Mutaqin, Thomas Edwin Santosa"),
"it"    => array("0","01","it","Italian","Luca Berti"),
"ja"    => array("0","10","ja","Japanese","Daisuke Deguchi (GUI), Kurasawa Nozomu (docs)"),
"ko"    => array("0","00","ko","Korean","Hojin Choi, Jae-Hong Lee"),
"ka"    => array("0","00","ka","Georgian","Gia Shervashidze"),
"mk"    => array("0","00","mk","Macedonian","Srdjan Stokic"),
"nb"    => array("0","01","nb","Norwegian, bokmål","Jostein Andersen, Oddvar Kloster"),
"nl"    => array("0","01","nl","Dutch","Jean-Marc van Leerdam, Yves Reynhout"),
"pl"    => array("0","01","pl","Polish","Michal Mielniczuk, Andrzej Rusin, Piotr Kontek"),
"pt_BR" => array("0","01","pt_BR","Portuguese, Brazil","Humberto Osório, Guilherme Cantisano, Juliano Ravasi Ferraz, Victor Westmann, Conrado Quilles Gomes"),
"pt_PT" => array("0","00","pt_PT","Portuguese, Portugal","André Malafaya Baptista"),
"ro"    => array("0","00","ro","Romanian","Seby Rusu"),
"ru"    => array("0","11","ru","Russian","Dzmitry Lazerka, Dimitry Mardiyan, Shura Zam, Alexander Leschinsky, Stanislav Petrakov"),
"sk"    => array("0","11","sk","Slovak","Oto Brezina"),
"sl"    => array("0","11","sl","Slovenian","Goran Kavrecic, Martin Srebotnjak, Matjaz Cepon"),
"sv"    => array("0","01","sv","Swedish","Johan Stenqvist, Johan Appelgren"),
"tr"    => array("0","00","tr","Turkish","Aydogan Yolacan"),
"zh_CN" => array("0","10","zh_CN","Chinese, simplified","Dongsheng Song, Cecil Sheng, Ken Chew, Cobra Net, Xingyu Wang, Lixinfeng"),
"zh_TW" => array("0","00","zh_TW","Chinese, traditional","Cecil Sheng"),
"zzz"   => array("1","00","gb","Empty Catalog (English)","Stefan Küng, Simon Large, Lübbe Onken")
);
?>
