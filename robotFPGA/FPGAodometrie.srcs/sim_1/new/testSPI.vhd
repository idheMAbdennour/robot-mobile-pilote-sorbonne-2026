library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

entity tb_SPI is
-- Un testbench n'a jamais de ports d'entrées/sorties
end tb_SPI;

architecture Behavioral of tb_SPI is

    -- 2. Signaux internes de liaison
    signal clk100 : std_logic := '0';
    signal rst    : std_logic := '0';
    signal CS     : std_logic_vector(3 downto 0) := "1111";
    signal SCK    : std_logic := '0';
    signal MISO   : std_logic;
    
    signal Vg : std_logic_vector(27 downto 0) := X"1234567"; 
    signal Vd : std_logic_vector(27 downto 0) := X"A5A5A5A"; 

    -- Pour 8 bits, on utilise 2 caractères hexadécimaux (2 x 4 = 8 bits)
    signal Pg : std_logic_vector(7 downto 0)  := X"FF";
    signal Pd : std_logic_vector(7 downto 0)  := X"00";

    -- Constantes de temps
    constant CLK_PERIOD : time := 10 ns;  -- Horloge FPGA à 100 MHz
    constant SPI_PERIOD : time := 1000 ns; -- Horloge SPI à 1 MHz (Bit-Banging)

begin

    -- 3. Instanciation de l'UUT
    uut: entity work.SPI port map (
          clk100 => clk100,
          rst    => rst,
          CS     => CS,
          SCK    => SCK,
          MISO   => MISO,
          Vg     => Vg,
          Vd     => Vd,
          Pg     => Pg,
          Pd     => Pd
        );

    -- 4. Génération de l'horloge globale FPGA (100 MHz)
    clk_process : process
    begin
        clk100 <= '0';
        wait for CLK_PERIOD/2;
        clk100 <= '1';
        wait for CLK_PERIOD/2;
    end process;

    -- 5. Process de simulation (Scénario de test)
    stim_proc: process
    begin		
        -- Phase de Reset initial
        rst <= '1';
        wait for 100 ns;
        rst <= '0';
        wait for 100 ns;

        ----------------------------------------------------------------
        -- SCÉNARIO 1 : Lecture du registre Vg (8 bits étendu à 32)
        ----------------------------------------------------------------
        CS <= "1110"; 
        wait for SPI_PERIOD;

        for i in 0 to 31 loop
            SCK <= '0';
            wait for SPI_PERIOD/2;
            SCK <= '1';            
            wait for SPI_PERIOD/2;
        end loop;
        
        SCK <= '0';
        wait for 200 ns;
        CS <= "1111"; 
        
        wait for 2 us;

        ----------------------------------------------------------------
        -- SCÉNARIO 2 : Lecture du registre Pg (28 bits étendu à 32)
        ----------------------------------------------------------------
        -- Le Master active le CS pour Pg
        CS <= "1011"; 
        wait for SPI_PERIOD;

        -- On génère les 32 impulsions d'horloge
        for i in 0 to 31 loop
            SCK <= '0';
            wait for SPI_PERIOD/2;
            SCK <= '1'; -- Le FPGA va sortir les bits un par un
            wait for SPI_PERIOD/2;
        end loop;
        
        SCK <= '0';
        wait for 200 ns;
        CS <= "1111";

        -- Fin de la simulation
        wait;
    end process;

end Behavioral;