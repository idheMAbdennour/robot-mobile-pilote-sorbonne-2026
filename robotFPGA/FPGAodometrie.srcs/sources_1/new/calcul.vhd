library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

entity odometrie_globale is
    Port (
        clk, rst, qa_g,  qb_g, qa_d, qb_d: in std_logic;
        Pg, Pd: out std_logic_vector(7 downto 0);
        Vg, Vd: out std_logic_vector(27 downto 0);
        top50: out std_logic
    );
end odometrie_globale;

architecture Behavioral of odometrie_globale is

    signal s_top50, s_top50_prec: STD_LOGIC;
    signal s_pas_g, s_pas_d : STD_LOGIC_VECTOR(8 downto 0);
    signal s_top_t_g, s_top_t_d : STD_LOGIC;

    signal s_pas_g_prec, s_pas_d_prec : STD_LOGIC_VECTOR(8 downto 0) := (others => '0');
    signal chg_pas_g, chg_pas_d : STD_LOGIC := '0';
    signal acc_p_g, acc_p_d : STD_LOGIC_VECTOR(7 downto 0) := (others => '0');

    signal cpt_166ms   : STD_LOGIC_VECTOR(23 downto 0) := (others => '0');
    signal tick_166ms  : STD_LOGIC := '0';
    signal acc_v_g, acc_v_d : STD_LOGIC_VECTOR(15 downto 0) := (others => '0');

begin

    u_timer50 : entity work.timer50
        port map ( clk100 => clk, rst => rst, top50 => s_top50);
    top50 <= s_top50;

    u_cpt_g : entity work.compteur_tour(Behavioral)
        port map ( clk => clk, rst => rst, qa => qa_g, qb => qb_g, pas_count => s_pas_g, top_tour => s_top_t_g );

    u_cpt_d : entity work.compteur_tour(Behavioral)
        port map ( clk => clk, rst => rst, qa => qa_d, qb => qb_d, pas_count => s_pas_d, top_tour => s_top_t_d );

    process(clk)
    begin
        if rising_edge(clk) then
            if rst = '1' then
                s_pas_g_prec <= (others => '0'); s_pas_d_prec <= (others => '0');
                chg_pas_g <= '0'; chg_pas_d <= '0';
            else
                s_pas_g_prec <= s_pas_g; s_pas_d_prec <= s_pas_d;
                if s_pas_g /= s_pas_g_prec then chg_pas_g <= '1'; else chg_pas_g <= '0'; end if;
                if s_pas_d /= s_pas_d_prec then chg_pas_d <= '1'; else chg_pas_d <= '0'; end if;
            end if;
        end if;
    end process;

    process(clk)
    begin
        if rising_edge(clk) then
            if rst = '1' then
                acc_p_g <= (others => '0'); acc_p_d <= (others => '0');
                Pg <= (others => '0'); Pd <= (others => '0');
                s_top50_prec <= '0';
            else
                s_top50_prec <= s_top50;
                if chg_pas_g = '1' then acc_p_g <= acc_p_g + 1; end if;
                if chg_pas_d = '1' then acc_p_d <= acc_p_d + 1; end if;
                if s_top50 = '1' and s_top50_prec = '0' then
                    Pg <= acc_p_g; Pd <= acc_p_d;
                    acc_p_g <= (others => '0'); acc_p_d <= (others => '0');
                end if;
            end if;
        end if;
    end process;

    process(clk)
    begin
        if rising_edge(clk) then
            if rst = '1' then
                cpt_166ms <= (others => '0'); tick_166ms <= '0';
            elsif cpt_166ms = 16666666 then
                cpt_166ms <= (others => '0'); tick_166ms <= '1';
            else
                cpt_166ms <= cpt_166ms + 1; tick_166ms <= '0';
            end if;
        end if;
    end process;

    -- 1RPM = 360 pas / 60s = 6 pas/s
    process(clk)
    begin
        if rising_edge(clk) then
            if rst = '1' then
                acc_v_g <= (others => '0'); acc_v_d <= (others => '0');
                Vg <= (others => '0'); Vd <= (others => '0');
            else
                if chg_pas_g = '1' then acc_v_g <= acc_v_g + 1; end if;
                if chg_pas_d = '1' then acc_v_d <= acc_v_d + 1; end if;

                if tick_166ms = '1' then
                    Vg <= X"000" & acc_v_g;
                    Vd <= X"000" & acc_v_d;
                    acc_v_g <= (others => '0'); acc_v_d <= (others => '0');
                end if;
            end if;
        end if;
    end process;

end Behavioral;