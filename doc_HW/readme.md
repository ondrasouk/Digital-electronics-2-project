# Dokumentace HW a knihoven
# Snímání hladiny v nádrži
Snímání je hotové. Práce s funkcemi viz komentáře doxygenu.

Senzorů je celkem 5. Takže v 25ti litrovem kanistru jest rozlišení přibližně po 5-ti litrech. Jsou snímaný digitálně (rozlišuji jen dva stavy) z praktických testů není problem s dostatečně nízkou rezistivitou vody...


# Snímání vlhkosti půdy
Snímání vlhkosti půdy je řešeno rezistivní sondou - dva velké hřebíky zapíchnuté do hlíny. Odporová změna mezi vodiči probíhá cca od 0-10kOhm/cm v závislosti na vlhkosti půdy. Druhý odpor děliče tvoří pro jednoduchost pullup rezistor v MCU. Ten má velikost od 30-50kOhm

Protože sondy (hřebíky)jsou z klasické oceli (jinak se používají nerezové trny), může dochízek ke korozi. Elektrolýza velmi výrazně napománá ke vzniku koroze na povrchu vodičů jimiž proudí DC proud. Ke zpomalení vlivu koroze se tedy vnitřní pullup rezistor připíná pouze při měření.