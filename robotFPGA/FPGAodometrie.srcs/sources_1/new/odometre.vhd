library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

entity compteur_tour is
    Port (
        clk, rst, qa, qb: in std_logic;
        top_tour: out std_logic;
        pas_count: out std_logic_vector(8 downto 0)
    );
end compteur_tour;

architecture Behavioral of compteur_tour is
    signal qa_sync : std_logic_vector (2 downto 0) := (others => '0');
    signal qb_sync : std_logic_vector (2 downto 0) := (others => '0');
    
    signal qa_stable : std_logic := '0';
    signal qb_stable : std_logic := '0';
    
    signal qa_stable_prec : std_logic := '0';
    signal qb_stable_prec : std_logic := '0';
    
    signal any_edge   : std_logic := '0';
    signal sens_avant : std_logic := '0';
    
    signal counter : std_logic_vector (8 downto 0) := (others => '0');

begin

    -- Sync
    process(clk)
    begin
        if rising_edge(clk) then
            if rst = '1' then
                qa_sync <= (others => '0');
                qb_sync <= (others => '0');
                qa_stable <= '0';
                qb_stable <= '0';
            else
                qa_sync <= qa_sync(1 downto 0) & qa;
                qb_sync <= qb_sync(1 downto 0) & qb;
                
                -- Stable pendant 3 tick de 100MHz
                if qa_sync = "111" then qa_stable <= '1'; elsif qa_sync = "000" then qa_stable <= '0'; end if;
                if qb_sync = "111" then qb_stable <= '1'; elsif qb_sync = "000" then qb_stable <= '0'; end if;
            end if;
        end if;
    end process;

    -- Détection
    process(clk)
    begin
        if rising_edge(clk) then
            if rst = '1' then
                qa_stable_prec <= '0';
                qb_stable_prec <= '0';
                any_edge       <= '0';
                sens_avant     <= '0';
            else
                qa_stable_prec <= qa_stable;
                qb_stable_prec <= qb_stable;
                
                any_edge <= (qa_stable xor qa_stable_prec) or (qb_stable xor qb_stable_prec);
                
                sens_avant <= qa_stable xor qb_stable_prec;
            end if;
        end if;
    end process;

    -- 3. Comptage sur 360 pas
    process(clk)
    begin
        if rising_edge(clk) then
            if rst = '1' then
                counter  <= (others => '0');
                top_tour <= '0';
            else
                top_tour <= '0'; 
                
                if any_edge = '1' then
                    if sens_avant = '1' then
                        -- Marche avant
                        if counter = "101101000" then -- 360
                            counter <= (others => '0');
                            top_tour <= '1';
                        else
                            counter <= counter + 1;
                        end if;
                    else
                        -- Marche arrière
                        if counter = "000000000" then
                            counter <= "101101000";
                            top_tour <= '1';
                        else
                            counter <= counter - 1;
                        end if;
                    end if;
                end if;
            end if;
        end if;
    end process;

    pas_count <= counter;

end Behavioral;