## ====================================================================
## HORLOGE ET RESET (Bouton Central)
## ====================================================================
set_property PACKAGE_PIN W5 [get_ports clk]
set_property IOSTANDARD LVCMOS33 [get_ports clk]
create_clock -add -name sys_clk_pin -period 10.00 -waveform {0 5} [get_ports clk]

set_property PACKAGE_PIN U18 [get_ports rst]
set_property IOSTANDARD LVCMOS33 [get_ports rst]

## ====================================================================
## SWITCHES
## ====================================================================
set_property PACKAGE_PIN V17 [get_ports {sw[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {sw[0]}]
set_property PACKAGE_PIN V16 [get_ports {sw[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {sw[1]}]

## ====================================================================
## AFFICHEURS 7 SEGMENTS (Anodes et Cathodes)
## ====================================================================
set_property PACKAGE_PIN W7 [get_ports {seg[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {seg[0]}]
set_property PACKAGE_PIN W6 [get_ports {seg[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {seg[1]}]
set_property PACKAGE_PIN U8 [get_ports {seg[2]}]
set_property IOSTANDARD LVCMOS33 [get_ports {seg[2]}]
set_property PACKAGE_PIN V8 [get_ports {seg[3]}]
set_property IOSTANDARD LVCMOS33 [get_ports {seg[3]}]
set_property PACKAGE_PIN U5 [get_ports {seg[4]}]
set_property IOSTANDARD LVCMOS33 [get_ports {seg[4]}]
set_property PACKAGE_PIN V5 [get_ports {seg[5]}]
set_property IOSTANDARD LVCMOS33 [get_ports {seg[5]}]
set_property PACKAGE_PIN U7 [get_ports {seg[6]}]
set_property IOSTANDARD LVCMOS33 [get_ports {seg[6]}]

set_property PACKAGE_PIN U2 [get_ports {an[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {an[0]}]
set_property PACKAGE_PIN U4 [get_ports {an[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {an[1]}]
set_property PACKAGE_PIN V4 [get_ports {an[2]}]
set_property IOSTANDARD LVCMOS33 [get_ports {an[2]}]
set_property PACKAGE_PIN W4 [get_ports {an[3]}]
set_property IOSTANDARD LVCMOS33 [get_ports {an[3]}]

## ====================================================================
## CONNECTEUR JA : ENTRÉES CODEURS (3.3V Fixe)
## ====================================================================
# Codeur Gauche - QA (Broche JA1)
set_property PACKAGE_PIN J1 [get_ports qa_g]
set_property IOSTANDARD LVCMOS33 [get_ports qa_g]
set_property PULLUP true [get_ports qa_g]

# Codeur Gauche - QB (Broche JA2)
set_property PACKAGE_PIN L2 [get_ports qb_g]
set_property IOSTANDARD LVCMOS33 [get_ports qb_g]
set_property PULLUP true [get_ports qb_g]

# Codeur Droite - QA (Broche JA3)
set_property PACKAGE_PIN J2 [get_ports qa_d]
set_property IOSTANDARD LVCMOS33 [get_ports qa_d]
set_property PULLUP true [get_ports qa_d]

# Codeur Droite - QB (Broche JA4)
set_property PACKAGE_PIN G2 [get_ports qb_d]
set_property IOSTANDARD LVCMOS33 [get_ports qb_d]
set_property PULLUP true [get_ports qb_d]

## ====================================================================
## CONNECTEUR JC : BUS DE COMMUNICATION SPI COMPLET (Tout en Banque 15 - 3.3V)
## ====================================================================
# CS(0) - Sélection Vg (Broche JC1)
set_property PACKAGE_PIN K17 [get_ports {CS[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {CS[0]}]

# CS(1) - Sélection Vd (Broche JC2)
set_property PACKAGE_PIN M18 [get_ports {CS[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {CS[1]}]

# CS(2) - Sélection Pg (Broche JC3)
set_property PACKAGE_PIN N17 [get_ports {CS[2]}]
set_property IOSTANDARD LVCMOS33 [get_ports {CS[2]}]

# CS(3) - Sélection Pd (Broche JC4)
set_property PACKAGE_PIN P18 [get_ports {CS[3]}]
set_property IOSTANDARD LVCMOS33 [get_ports {CS[3]}]

# Horloge SPI - SCK (Broche JC7)
set_property PACKAGE_PIN L17 [get_ports SCK]
set_property IOSTANDARD LVCMOS33 [get_ports SCK]

# Donnée SPI - MISO (Broche JC8)
set_property PACKAGE_PIN M19 [get_ports MISO]
set_property IOSTANDARD LVCMOS33 [get_ports MISO]

# Sortie témoin Top 50Hz (Broche JC9)
set_property PACKAGE_PIN P17 [get_ports top50]
set_property IOSTANDARD LVCMOS33 [get_ports top50]