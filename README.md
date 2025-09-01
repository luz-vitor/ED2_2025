# Processamento de Imagens PGM (P2 e P5)

Este projeto implementa um programa em C para **ler, salvar e processar imagens no formato PGM** (Portable Gray Map), tanto em **texto (P2)** quanto em **binário (P5)**.  
O processamento aplicado é a **limiarização**, que transforma a imagem em **preto e branco** com base em um valor de limiar definido pelo usuário.

---

## 📌 Funcionalidades

- Ler imagens **PGM no formato texto (P2)**.  
- Salvar imagens em **PGM texto (P2)** e **PGM binário (P5)**.  
- Ler imagens **PGM no formato binário (P5)**.  
- Aplicar **limiarização** com um valor `L`:  
  - Pixels `< L` recebem valor `0` (preto).  
  - Pixels `≥ L` recebem `maxIntensidade` (branco).  
- Gerar arquivos de saída no formato **binário (P5)**.  

---

## 🖼️ Exemplo de Fluxo

1. Ler a imagem `exemplo.pgm` (texto - P2).  
2. Converter e salvar em `exemplo_binaria.pgm` (binário - P5).  
3. Reabrir a versão binária.  
4. Aplicar **limiarização** com `L = 128`.  
5. Salvar em `exemplo_limiarizada.pgm` (binário - P5).  

---

## 📂 Estrutura do Código

- **`PGMImage`** → Estrutura que armazena os metadados e a matriz de pixels.  
- **`lerPGM`** → Lê imagem em texto (P2).  
- **`salvarPGM`** → Salva imagem em texto (P2).  
- **`salvarPGMBinario`** → Salva imagem em binário (P5).  
- **`lerPGMBinario`** → Lê imagem em binário (P5).  
- **`limiarizacao`** → Aplica o processo de limiarização.  
- **`main`** → Exemplo de uso (entrada → conversão → limiarização → saída).  

---

## 🚀 Compilação e Execução

No terminal, compile com `gcc`:

```bash
gcc main.c -o processamento 
```

Execute o programa:

```bash
./processamento
```

---

## 📁 Estrutura de Arquivos Esperada

```
imagens/
├── exemplo.pgm              # entrada em P2
├── exemplo_binaria.pgm      # saída em P5 (cópia da entrada)
└── exemplo_limiarizada.pgm  # saída em P5 (limiarizada)
```

---

## ⚠️ Observações

- O código **não trata comentários (`#`) no cabeçalho PGM**. Se o arquivo possuir comentários, pode ser necessário removê-los manualmente.  
- O programa assume que o valor máximo de intensidade (`maxIntensidade`) é **≤ 255** (1 byte por pixel).  
- Os arquivos `.pgm` devem estar dentro da pasta `imagens/`.  

---

## 📚 Referência

Formato PGM (Portable Gray Map):  
- [Especificação Netpbm](https://netpbm.sourceforge.net/doc/pgm.html)

- Readme.md gerado com a ajuda do ChatGPT 🤖
