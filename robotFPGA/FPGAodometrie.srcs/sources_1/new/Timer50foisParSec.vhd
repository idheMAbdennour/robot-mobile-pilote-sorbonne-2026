library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

entity timer50 is
    Port ( 
        clk100, rst: in  std_logic;
        top50: out std_logic
    );
end timer50;

architecture Behavioral of timer50 is
    signal cpt : std_logic_vector(20 downto 0) := (others => '0');
begin

    process (clk100)
    begin
        if rising_edge(clk100) then
            if rst = '1' then
                cpt <= (others => '0');
            else
                if cpt = 1999999 then
                    cpt <= (others => '0');
                    top50 <= '1'; 
                else
                    cpt <= cpt + 1;
                    
                    if cpt = 99999 then 
                        top50 <= '0';
                    end if;
                end if;
            end if;
        end if;
    end process;

end Behavioral;