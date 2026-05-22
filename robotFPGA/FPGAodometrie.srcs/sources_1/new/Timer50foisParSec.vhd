----------------------------------------------------------------------------------
-- Company: ouais
-- Engineer: ouais
-- 
-- Create Date: 21.05.2026 15:48:48
-- Design Name: Alexandre
-- Module Name: div 2000 clk - Behavioral
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
use IEEE.std_logic_unsigned.ALL;

entity Timer50foisParSec is
Port (  clk50kHz: in std_logic;
        res: out std_logic
 );
end Timer50foisParSec;

architecture Behavioral of Timer50foisParSec is
    signal cpt: std_logic_vector(10 downto 0) := (others => '0');
begin
    
    process (clk50kHz)
    begin
        if (rising_edge(clk50kHz)) then
        
            if cpt = 2000 then
                 cpt <= (others => '0');
            else
                cpt <= cpt + 1;
            end if;
            
        end if;
    end process;
    
    res <= '1' when cpt = 2000 else '0';

end Behavioral;
