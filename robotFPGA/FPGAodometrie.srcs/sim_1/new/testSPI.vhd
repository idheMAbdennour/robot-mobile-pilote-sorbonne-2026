----------------------------------------------------------------------------------
-- Company: ouais
-- Engineer: ouais
-- 
-- Create Date: 21.05.2026 15:48:48
-- Design Name: Alexandre
-- Module Name: test du SPI - Behavioral
-- Project Name: Projet L3 - Groupe Mehdi
-- Target Devices: FPGA
-- Tool Versions: ouais
-- Description: ouais
-- 
-- Dependencies: ouais
-- 
-- Revision:
-- Revision 0.01 - File Created
-- Additional Comments:
-- 
----------------------------------------------------------------------------------

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

entity testSPI is
end testSPI;

architecture Behavioral of testSPI is
    signal clk100KHz: std_logic := '0';
    signal cs: std_logic_vector(3 downto 0) := (others => '0');
    signal MISO: std_logic;
    signal Vg, Vd, Pg, Pd: std_logic_vector(7 downto 0) := (others => '0');
begin
    
    spiEnt: entity work.SPI(Behavioral)
        port map (clk100KHz => clk100KHz, cs => cs, MISO => MISO, Vg =>Vg, Vd => Vd, Pg => Pg, Pd => Pd);
       
    clk100KHz <= not(clk100KHz) after 10ns;
    
    cs(0) <= '1', '0' after 15 ns, '1' after 50 ns, '0' after 62 ns;
    cs(1) <= '0', '1' after 200 ns, '0' after 212ns;
    
    
    Vg(0) <= '1';
    Vg(1) <= '1'; 
    Vg(3) <= '1'; 
    Vg(5) <= '1'; 
    Vg(7) <= '1'; 
    
    Vd(0) <= '1';
    Vd(2) <= '1'; 
    Vd(4) <= '1'; 
    Vd(6) <= '1'; 
    Vd(7) <= '1'; 
                
    --cs(1) <= not(cs(1)) after 70000ns;
    --cs(2) <= not(cs(2)) after 70000ns;
    --cs(3) <= not(cs(3)) after 70000ns;
    

end Behavioral;
