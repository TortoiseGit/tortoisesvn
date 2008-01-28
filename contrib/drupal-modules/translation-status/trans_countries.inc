<?php
// Meaning of the columns:
// 1: 1 = pot file, 0 = po file
// 2: Flag byte used to create links on different download pages
//    !!! Bit0 is the rightmost bit !!!
//    Bit0 = spellchecker available
//    Bit1 = TortoiseSVN manual available
//    Bit2 = TortoiseMerge manual available
// 3: Country code. Used to display the flag. The [flagname].png has to match this column
// 4: Full country name.
// 5: List of translators.
$countries = array(
"bg"    => array("0","001","bg","Bulgarian","Milen A. Radev"),
"bs_BA" => array("0","000","bs_BA","Bosnian/Herzegovinan","Boris Musa"),
"ca_ES" => array("0","001","ca_ES","Catalan","Arnau Perelló"),
"cs"    => array("0","001","cs","Czech","Radek Svarz, Vojtech Vondra"),
"da"    => array("0","001","da","Danish","Jan Normann Nielsen, Gunther Strube"),
"de"    => array("0","111","de","German","Stefan Küng, Lübbe Onken"),
"el"    => array("0","000","el","Greek","George Marselis"),
"eo"    => array("0","000","eo","Esperanto","Tim Morley"),
"es"    => array("0","111","es","Spanish","Fernando P. Nájera Cano, Antonio Romero Lopez"),
"fa"    => array("0","000","fa","Farsi (Persian)","Titto Agustine"),
"fr"    => array("0","111","fr","French","Olivier Booklage, Philippe Torche, Jean-Christophe Gigogne, Jérémy Badier"),
"fi"    => array("0","110","fi","Finnish","Pekka Järvinen, Kari Granö"),
"hr"    => array("0","101","hr","Croatian","Helix d.o.o: Dinko Srkoc, Mario Saric"),
"hu"    => array("0","001","hu","Hungarian","Szente-Varga Bálint"),
"id"    => array("0","111","id","Indonesian","Zaenal Mutaqin, Thomas Edwin Santosa"),
"it"    => array("0","001","it","Italian","Luca Berti"),
"ja"    => array("0","110","ja","Japanese","Daisuke Deguchi (GUI), Kurasawa Nozomu (docs)"),
"ko"    => array("0","000","ko","Korean","Hojin Choi, Jae-Hong Lee"),
"ka"    => array("0","000","ka","Georgian","Gia Shervashidze"),
"mk"    => array("0","000","mk","Macedonian","Srdjan Stokic"),
"nb"    => array("0","001","nb","Norwegian, bokmål","Jostein Andersen, Oddvar Kloster"),
"nl"    => array("0","101","nl","Dutch","Jean-Marc van Leerdam, Yves Reynhout, Bob Hulst"),
"pl"    => array("0","001","pl","Polish","Michal Mielniczuk, Andrzej Rusin, Piotr Kontek"),
"pt_BR" => array("0","001","pt_BR","Portuguese, Brazil","Humberto Osório, Guilherme Cantisano, Juliano Ravasi Ferraz, Victor Westmann, Conrado Quilles Gomes, Tiago Deliberali Santos"),
"pt_PT" => array("0","000","pt_PT","Portuguese, Portugal","André Malafaya Baptista"),
"ro"    => array("0","000","ro","Romanian","Seby Rusu"),
"ru"    => array("0","111","ru","Russian","Dzmitry Lazerka, Dimitry Mardiyan, Shura Zam, Alexander Leschinsky, Stanislav Petrakov"),
"sk"    => array("0","111","sk","Slovak","Oto Brezina"),
"sl"    => array("0","111","sl","Slovenian","Goran Kavrecic, Martin Srebotnjak, Matjaz Cepon"),
"sv"    => array("0","001","sv","Swedish","Johan Stenqvist, Johan Appelgren"),
"tr"    => array("0","000","tr","Turkish","Aydogan Yolacan"),
"zh_CN" => array("0","110","zh_CN","Chinese, simplified","Dongsheng Song, Cecil Sheng, Ken Chew, Cobra Net, Xingyu Wang, Lixinfeng"),
"zh_TW" => array("0","000","zh_TW","Chinese, traditional","Cecil Sheng"),
"zzz"   => array("1","000","gb","Empty Catalog (English)","Stefan Küng, Simon Large, Lübbe Onken")
);
?>
