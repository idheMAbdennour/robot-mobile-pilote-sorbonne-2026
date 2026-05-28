library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

entity top is
    Port (
        clk, rst, qa_g, qb_g, qa_d, qb_d, SCK: in STD_LOGIC;     
        sw: in  STD_LOGIC_VECTOR(1 downto 0); 
        CS: in std_logic_vector(3 downto 0);
        seg: out STD_LOGIC_VECTOR(6 downto 0); 
        an: out STD_LOGIC_VECTOR(3 downto 0); 
        top50, MISO: out STD_LOGIC                     
    );
end top;

architecture Behavioral of top is

    signal s_top50 : STD_LOGIC;
    signal s_Pg, s_Pd  : STD_LOGIC_VECTOR(7 downto 0);
    signal s_Vg, s_Vd  : STD_LOGIC_VECTOR(27 downto 0);

    signal data_disp   : STD_LOGIC_VECTOR(15 downto 0);
    
    signal bcd_thousands : STD_LOGIC_VECTOR(3 downto 0);
    signal bcd_hundreds  : STD_LOGIC_VECTOR(3 downto 0);
    signal bcd_tens      : STD_LOGIC_VECTOR(3 downto 0);
    signal bcd_units     : STD_LOGIC_VECTOR(3 downto 0);

    signal clk_div     : STD_LOGIC_VECTOR(19 downto 0) := (others => '0');
    signal digit_sel   : STD_LOGIC_VECTOR(1 downto 0);
    signal dec_val     : STD_LOGIC_VECTOR(3 downto 0);

begin
    u_odometre : entity work.odometrie_globale
        port map (
            clk => clk, rst => rst,
            qa_g => qa_g, qb_g => qb_g, qa_d => qa_d, qb_d => qb_d,
            Pg => s_Pg, Pd => s_Pd, Vg => s_Vg, Vd => s_Vd,
            top50 => s_top50
        );
    top50 <= s_top50;
    
    u_spi : entity work.SPI
        port map ( 
            clk100 => clk, rst => rst,
            CS => CS, SCK => SCK, MISO => MISO, Vg => s_Vg, Vd => s_Vd,
            Pg => s_Pg, Pd => s_Pd
        );

    process(sw, s_Pg, s_Pd, s_Vg, s_Vd)
    begin
        case sw is
            when "00"   => data_disp <= X"00" & s_Pg;
            when "01"   => data_disp <= X"00" & s_Pd;
            when "10"   => data_disp <= s_Vg(15 downto 0);
            when "11"   => data_disp <= s_Vd(15 downto 0);
            when others => data_disp <= (others => '0');
        end case;
    end process;

    process(data_disp)
        variable temp : std_logic_vector(13 downto 0);
        variable bcd  : std_logic_vector(15 downto 0);
    begin
        temp := data_disp(13 downto 0);
        bcd := (others => '0');
        
        for i in 0 to 13 loop
            if bcd(3 downto 0)   >= "0101" then bcd(3 downto 0)   := bcd(3 downto 0)   + "0011"; end if;
            if bcd(7 downto 4)   >= "0101" then bcd(7 downto 4)   := bcd(7 downto 4)   + "0011"; end if;
            if bcd(11 downto 8)  >= "0101" then bcd(11 downto 8)  := bcd(11 downto 8)  + "0011"; end if;
            if bcd(15 downto 12) >= "0101" then bcd(15 downto 12) := bcd(15 downto 12) + "0011"; end if;
            
            bcd  := bcd(14 downto 0) & temp(13);
            temp := temp(12 downto 0) & '0';
        end loop;
        
        bcd_thousands <= bcd(15 downto 12);
        bcd_hundreds  <= bcd(11 downto 8);
        bcd_tens      <= bcd(7 downto 4);
        bcd_units     <= bcd(3 downto 0);
    end process;

    process(clk)
    begin
        if rising_edge(clk) then clk_div <= clk_div + 1; end if;
    end process;
    
    digit_sel <= clk_div(19 downto 18);

    process(digit_sel, bcd_units, bcd_tens, bcd_hundreds, bcd_thousands)
    begin
        case digit_sel is
            when "00" => an <= "1110"; dec_val <= bcd_units;     -- Chiffre des unités (à droite)
            when "01" => an <= "1101"; dec_val <= bcd_tens;      -- Chiffre des dizaines
            when "10" => an <= "1011"; dec_val <= bcd_hundreds;  -- Chiffre des centaines
            when "11" => an <= "0111"; dec_val <= bcd_thousands; -- Chiffre des milliers (à gauche)
            when others => an <= "1111"; dec_val <= X"0";
        end case;
    end process;

    process(dec_val)
    begin
        case dec_val is
            when X"0" => seg <= "1000000"; -- 0
            when X"1" => seg <= "1111001"; -- 1
            when X"2" => seg <= "0100100"; -- 2
            when X"3" => seg <= "0110000"; -- 3
            when X"4" => seg <= "0011001"; -- 4
            when X"5" => seg <= "0010010"; -- 5
            when X"6" => seg <= "0000010"; -- 6
            when X"7" => seg <= "1111000"; -- 7
            when X"8" => seg <= "0000000"; -- 8
            when X"9" => seg <= "0010000"; -- 9
            when others => seg <= "1111111"; -- Éteint (Sécurité)
        end case;
    end process;

end Behavioral;