# Explicação do código – Organização e Acesso de Arquivos

## 1. Estrutura dos Arquivos

O programa desenvolvido utiliza dois arquivos distintos:

### a) Arquivo de Dados (`imagens.bin`)

- É importante lembrar que os comentários das imagens foram retirados manualmente para facilitar a escrita das imagens.
- Esse arquivo armazena somente os pixels das imagens, sem cabeçalhos de formato.
- Cada imagem é gravada sequencialmente, logo após a anterior, sem espaços em branco.
- Como diferentes imagens podem ter tamanhos diferentes, o programa calcula e registra o **offset** de cada uma (isto é, a distância em bytes desde o início do arquivo até o início da imagem).
- **Organização**: sequencial **não indexada**, apenas um fluxo contínuo de bytes.

### b) Arquivo de Índice (`index.bin`)

- Armazena registros com informações descritivas de cada imagem:
  - Nome da imagem (**chave secundária**).
  - Offset no arquivo de dados (**chave primária**).
  - Dimensões (linhas e colunas).
  - Quantidade de tons de cinza. 
  - Tamanho em bytes.
- **Organização**: sequencial **indexada**. Cada registro é um `struct` fixo.

---

## 2. Forma de Organização dos Registros

- **Arquivo de dados (`imagens.bin`)**: registros de **tamanho variável** (cada imagem ocupa um espaço proporcional ao seu número de pixels).
- **Arquivo de índice (`index.bin`)**: registros de **tamanho fixo**, implementados com `struct IndexEntry`.
- Cada imagem salva corresponde a:
  - um registro no índice
  - um bloco de dados no arquivo de imagens
---

## 3. Acesso

O acesso às imagens é feito em dois passos:

1. **Busca sequencial no índice pelo nome da imagem.**  
   - Complexidade: **O(N)**, pois percorre os registros em ordem até encontrar a chave.
   - Um ponto de se comentar é que, com um grande número de imagens, a busca, por ser sequencial, pode se tornar lenta.

2. **Acesso direto no arquivo de dados.**  
   - Com o registro localizado, usa-se o **offset** para acessar diretamente os pixels da imagem.  
   - Implementado via `fseek`, levando o ponteiro de leitura diretamente ao início da imagem.  

---

## 4. Operações de Processamento

Após recuperar a imagem, o usuário pode:

- **Exportar sem modificação**  
  - Os pixels são gravados em um novo arquivo `.pgm`.  

- **Exportar negativada**  
  - Aplica-se a transformação:  
    ```
    I'(i,j) = C - I(i,j)
    ```
    Onde **C** é o número de tons de cinza.  

- **Exportar limiarizada**  
  - Aplica-se a regra:  
    ```
    Se pixel > C/2 → pixel = C
    Caso contrário → pixel = 0
    ```

---

## 5. Vantagens da Organização Adotada

- **Armazenamento compacto**: não há espaços vazios entre imagens.  
- **Flexibilidade**: imagens de tamanhos diferentes podem ser armazenadas sem problema.  
- **Separação de dados e índice**: o arquivo de dados não precisa ser modificado para buscar imagens; o índice organiza os metadados.  
- **Acesso eficiente**:  
  - Busca sequencial no índice.  
  - Acesso direto ao conteúdo da imagem.  

---

## 6. Desvantagem da Organização Adotada
- **Busca sequencial**: Com muitas imagens, o acesso por meio de busca sequencial pode se tornar lento.

---

## 7. Conclusão

O programa implementado aplica os conceitos de:

- Organização **sequencial** (no arquivo de dados).  
- Índice auxiliar com **chaves primária e secundária** (no arquivo de índice).  
- **Acesso sequencial e direto combinados**:  
  - Sequencial para localizar a imagem no índice.  
  - Direto para recuperar os pixels no arquivo binário.  
