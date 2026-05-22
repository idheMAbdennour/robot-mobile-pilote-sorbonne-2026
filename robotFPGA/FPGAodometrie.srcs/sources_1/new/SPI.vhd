----------------------------------------------------------------------------------
-- Company: ouais
-- Engineer: ouais
-- 
-- Create Date: 21.05.2026 15:48:48
-- Design Name: Alexandre
-- Module Name: SPI - Behavioral
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

entity SPI is
port ( 
    clk100KHz: in std_logic; -- Aussi à sortir avec le xdc
    cs: in std_logic_vector(3 downto 0);
    MISO: out std_logic := '0';
    Vg, Vd, Pg, Pd: in std_logic_vector(7 downto 0)
);
end SPI;

architecture Behavioral of SPI is
    signal resTimer: std_logic := '0';
    signal toSendTimerRes: std_logic := '0';
    
    signal nbBitToSend: natural;
    signal bitsToSend: std_logic_vector (64 downto 0) := (others => '0');
    signal readPosBitToSend: natural;
    signal writePosBitToSend: natural;
    
    signal misoTemp: std_logic := '0';
begin
    -- Timer 50 fois par sec à '1'
    timerDiv2000: entity work.Timer50foisParSec(Behavioral)
        port map (clk50kHz => clk100KHz, res => resTimer);
    
    clkChanged: process (clk100KHz)
        variable temp: natural;
    begin
        
        if (rising_edge(clk100KHz)) then
            misoTemp <= '0';
            
            -- Send message
            if (not(nbBitToSend = 0)) then
                nbBitToSend <= nbBitToSend - 1;
                misoTemp <= bitsToSend(readPosBitToSend);
                readPosBitToSend <= readPosBitToSend + 1;
                 if (readPosBitToSend = 64) then
                    readPosBitToSend <= 0;
                end if;
            end if;
            
            -- Nouvelle information -> informer le FPGA
            if( resTimer = '1' or toSendTimerRes = '1') then
                if (nbBitToSend = 0) then -- Envoie rien
                    if ( cs = "0000" ) then -- Rien n'est demander
                        misoTemp <= '1';
                        toSendTimerRes <= '0';
                    else
                        toSendTimerRes <= '1';
                    end if;
                else
                    toSendTimerRes <= '1';
                end if;
            end if;
            
            -- Le LPC fait une demande
            if (cs(0) = '1') then
                for i in 0 to 7 loop
                    bitsToSend(writePosBitToSend + i) <= Vg(i);
                end loop;
            elsif (cs(1) = '1') then
                for i in 0 to 7 loop
                    bitsToSend(writePosBitToSend + i) <= Vd(i);
                end loop;
            elsif (cs(2) = '1') then
                for i in 0 to 7 loop
                    bitsToSend(writePosBitToSend + i) <= Pg(i);
                end loop;
            elsif (cs(3) = '1') then
                for i in 0 to 7 loop
                    bitsToSend(writePosBitToSend + i) <= Pd(i);
                end loop;
            end if;
            
            if (not(cs = 0)) then
                writePosBitToSend <= writePosBitToSend + 8;
                nbBitToSend <= nbBitToSend + 8;
                if (writePosBitToSend = 56) then
                    writePosBitToSend <= 0;
                end if;
            end if;
            
            miso <= misoTemp;
        end if;
        
    end process;
    
    

end Behavioral;
