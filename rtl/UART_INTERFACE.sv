module UART_INTERFACE (
    input  logic       clk,
    input  logic       rst_n,
    input  logic       uart_rxd,
    output logic       uart_txd,
    
    output logic [2:0] data_ready, // Сигнал-флаг операции для контроллера памяти
    output logic [7:0] data_in,     // Передаваемый байт данных/команды
    
    input  logic [7:0] data_out,    // Байт, прочитанный из DRAM
    input  logic       tx_start     // Сигнал на отправку байта в ПК
);

    //---Состояния машины_--//
    typedef enum logic [1:0] { 
        IDLE,       // Ожидание старт-бита или команды
        RECEIVE,    // Прием байта
        TRANSMIT    // Отправка байта обратно в ПК
    } state_t;

    state_t state;

    logic [3:0]  bit_cnt;   // Счетчик бит в посылке
    logic [8:0]  clk_div;   // Делитель частоты для формирования бодрейта (50МГц / 115200)
    logic [7:0]  shift_reg; // Регистр сдвига данных
    logic        tx_reg;

    assign uart_txd = tx_reg;

    //---Машина состояний интерфейса_--//
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state      <= IDLE;
            tx_reg     <= 1'b1;
            data_ready <= '0;
            data_in    <= '0;
            clk_div    <= '0;
            bit_cnt    <= '0;
            shift_reg  <= '0;
        end else begin
            data_ready <= '0; // Импульсный сброс флага готовности

            case (state)
                IDLE: begin
                    tx_reg  <= 1'b1;
                    clk_div <= '0;
                    
                    if (!uart_rxd) begin // Детект старт-бита (спад в 0)
                        state   <= RECEIVE;
                        bit_cnt <= 7;
                    end else if (tx_start) begin
                        state     <= TRANSMIT;
                        shift_reg <= data_out;
                        bit_cnt   <= 9; // Старт + 8 бит + стоп
                    end
                end

                RECEIVE: begin
                    if (clk_div == 434) begin // Середина бита на 115200 бод при 50МГц
                        clk_div <= '0;
                        shift_reg[0] <= uart_rxd;
                        
                        if (bit_cnt == 0) begin
                            state <= IDLE;
                            data_in <= shift_reg;
                            if (shift_reg == 8'h3C)      data_ready <= 3'h1; // Флаг: Принят маркер адреса
                            else if (shift_reg == 8'h1A) data_ready <= 3'h2; // Флаг: Принята команда записи
                            else if (shift_reg == 8'h2B) data_ready <= 3'h3; // Флаг: Принята команда чтения
                            else                         data_ready <= 3'h4; // Флаг: Приняты транзитные данные чистых векторов
                        end else begin
                            shift_reg <= {shift_reg[6:0], 1'b0};
                            bit_cnt   <= bit_cnt - 1'b1;
                        end
                    end else begin
                        clk_div <= clk_div + 1'b1;
                    end
                end

                TRANSMIT: begin
                    if (clk_div == 434) begin
                        clk_div <= '0;
                        if (bit_cnt == 0) state <= IDLE;
                        else bit_cnt <= bit_cnt - 1'b1;
                    end else begin
                        clk_div <= clk_div + 1'b1;
                    end
                end
                
                default: state <= IDLE;
            endcase
        end
    end

endmodule
