library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.std_logic_unsigned.ALL;

entity SPI is
port ( 
    clk100, rst: in std_logic;
    CS: in std_logic_vector(3 downto 0);
    SCK: in std_logic;
    MISO: out std_logic;
    Vg, Vd: in std_logic_vector(27 downto 0);
    Pg, Pd: in std_logic_vector(7 downto 0)
);
end SPI;

architecture Behavioral of SPI is
    type state_t is (IDLE, START, SEND, STOP);
    signal state: state_t := IDLE;

    signal cs_reg: std_logic_vector(3 downto 0);
    signal sck_reg, sck_reg_prec: std_logic;
    signal sdo_reg: std_logic_vector (31 downto 0) := (others => '0');
    signal bit_count: std_logic_vector (5 downto 0) := (others => '0');
begin

  process(clk100, rst) begin
    if rst = '1' then
        state <= IDLE;
        cs_reg <= "1111";
        sck_reg <= '0';
        sck_reg_prec <= '0';
        sdo_reg <= (others => '0');
        bit_count <= (others => '0');
    elsif rising_edge(clk100) then
        cs_reg <= CS;
        sck_reg_prec <= sck_reg;
        sck_reg <= SCK;
        case state is 
            when IDLE => 
                state <= START;
                bit_count <= (others => '0');
                case cs_reg is 
                    when "1110" => sdo_reg <= "0000" & Vg;
                    when "1101" => sdo_reg <= "0000" & Vd;
                    when "1011" => sdo_reg <=  X"000000" & Pg;
                    when "0111" => sdo_reg <=  X"000000" & Pd;
                    when others => state <= IDLE;
                end case;
            when START =>
                state <= SEND;
                MISO <= sdo_reg(conv_integer(bit_count));
                bit_count <= bit_count + 1;
            when SEND =>
                state <= SEND;
                if CS = "1111" then
                    state <= STOP;
                end if;
                -- Sample on rising edge on master end, will change when fall
                if sck_reg = '0' and sck_reg_prec = '1' then
                    if bit_count <= 31 then
                        MISO <= sdo_reg(conv_integer(bit_count));
                        bit_count <= bit_count + 1;
                    else
                        state <= STOP;
                    end if;
                end if;
            when STOP =>
                state <= IDLE;
                MISO <= 'Z';
        end case;
    end if;
  
  end process;

end Behavioral;
