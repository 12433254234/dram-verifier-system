module DRAM_CYCLE_CTRL (
    input  logic        clk,		 //Тактовый сигнал 50 МГц
    input  logic        rst_n,		 //Асинхронный сброс
    input  logic [2:0]  data_ready,  // Сигнал-флаг от UART_INTERFACE
    input  logic [7:0]  data_in,     // Данные из UART
    output logic [7:0]  data_out,    // Данные для отправки обратно в ПК
    output logic        tx_start,    // Импульс запуска отправки в ПК
    
    output logic [15:0] dram_addr,
    inout  wire [7:0]   dram_data,
    output logic        dram_ras_n,
    output logic        dram_cas_n,
    output logic        dram_we_n
);

	//---Состояния машины---//
    typedef enum logic [1:0] { 
        MEM_IDLE,
        MEM_ADDR_LATCH,
        MEM_WRITE,
        MEM_READ
    } mem_state_t;

    mem_state_t state;

    logic [23:0] addr_buffer;   // Буфер для накопления 3 байт адреса
    logic [1:0]  byte_idx;      // Индекс принимаемого байта адреса
    logic [7:0]  reg_wdata;     // Регистр защелки данных для записи
    logic [7:0]  reg_rdata;     // Регистр защелки считанных из памяти данных
    logic        dram_dir;      // Управление буфером шины (1 - вывод на чип, 0 - чтение)

    //---буфер данных---//
    assign dram_data = dram_dir ? reg_wdata : 8'hZZ;
    assign data_out  = reg_rdata;
    assign dram_addr = addr_buffer[15:0]; // Младший сегмент адреса на шину

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state       <= MEM_IDLE;
            addr_buffer <= '0;
            byte_idx    <= '0;
            reg_wdata   <= '0;
            reg_rdata   <= '0;
            dram_dir    <= 1'b0;
            dram_ras_n  <= 1'b1;
            dram_cas_n  <= 1'b1;
            dram_we_n   <= 1'b1;
            tx_start    <= 1'b0;
        end else begin
            tx_start <= 1'b0;

            case (state)
                MEM_IDLE: begin
                    dram_ras_n <= 1'b1;
                    dram_cas_n <= 1'b1;
                    dram_we_n  <= 1'b1;
                    dram_dir   <= 1'b0;
                    byte_idx   <= '0;

                    if (data_ready == 3'h1) begin
                        state <= MEM_ADDR_LATCH; // Переход к приему байт адреса
                    end else if (data_ready == 3'h2) begin
                        state <= MEM_WRITE;
                    end else if (data_ready == 3'h3) begin
                        state <= MEM_READ;
                    end
                end

                MEM_ADDR_LATCH: begin
                    if (data_ready == 3'h4) begin // Пришел очередной байт адреса
                        addr_buffer <= {addr_buffer[15:0], data_in};
                        if (byte_idx == 2) begin
                            state <= MEM_IDLE; // Собрали все 3 байта адреса
                        end else begin
                            byte_idx <= byte_idx + 1'b1;
                        end
                    end
                end

                MEM_WRITE: begin
                    if (data_ready == 3'h4) begin // Пришел байт тестового вектора
                        reg_wdata  <= data_in;
                        dram_dir   <= 1'b1; // Переключаем шину на вывод в чип
                        dram_we_n  <= 1'b0; // Активируем сигналы управления DRAM
                        dram_ras_n <= 1'b0;
                        dram_cas_n <= 1'b0;
                        state      <= MEM_IDLE;
                    end
                end

                MEM_READ: begin
                    dram_dir   <= 1'b0; // Шина строго в Z-состоянии (на ввод)
                    dram_we_n  <= 1'b1; // Чтение
                    dram_ras_n <= 1'b0; // Удерживаем стробы таймингов
                    dram_cas_n <= 1'b0;
                    
                    reg_rdata  <= dram_data; // Защелкиваем данные с физических ножек
                    tx_start   <= 1'b1;      // Даем импульс UART-модулю на отправку в ПК
                    state      <= MEM_IDLE;
                end
                
                default: state <= MEM_IDLE;
            endcase
        end
    end

endmodule
