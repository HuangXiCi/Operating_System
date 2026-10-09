document.addEventListener("DOMContentLoaded", () => {
  const codeBlocks = document.querySelectorAll(
    '.rst-content div[class^="highlight-"], .rst-content div[class*=" highlight-"]'
  );

  const fallbackCopy = (text) => {
    const textarea = document.createElement("textarea");
    textarea.value = text;
    textarea.setAttribute("readonly", "");
    textarea.style.position = "fixed";
    textarea.style.opacity = "0";
    document.body.appendChild(textarea);
    textarea.select();
    const copied = document.execCommand("copy");
    textarea.remove();
    return copied;
  };

  codeBlocks.forEach((block) => {
    const code = block.querySelector("pre");
    const caption = block.closest(".literal-block-wrapper")?.querySelector(":scope > .code-block-caption");
    const buttonContainer = caption || block;
    if (!code || buttonContainer.querySelector(":scope > .code-copy-button")) {
      return;
    }

    block.classList.add("code-copy-container");
    if (caption) {
      block.classList.add("has-code-caption");
      caption.classList.add("code-copy-caption");
    }
    code.querySelectorAll(".linenos").forEach((number) => {
      number.setAttribute("aria-hidden", "true");
    });

    const button = document.createElement("button");
    button.type = "button";
    button.className = "code-copy-button";
    button.textContent = "复制";
    button.title = "复制代码";
    button.setAttribute("aria-label", "复制代码到剪贴板");

    button.addEventListener("click", async () => {
      const copy = code.cloneNode(true);
      copy.querySelectorAll(".linenos").forEach((number) => number.remove());
      const text = copy.textContent.replace(/\n$/, "");
      let copied = false;

      try {
        if (navigator.clipboard && window.isSecureContext) {
          await navigator.clipboard.writeText(text);
          copied = true;
        } else {
          copied = fallbackCopy(text);
        }
      } catch (_error) {
        copied = fallbackCopy(text);
      }

      button.textContent = copied ? "已复制" : "复制失败";
      button.classList.toggle("is-copied", copied);
      button.classList.toggle("is-error", !copied);

      window.setTimeout(() => {
        button.textContent = "复制";
        button.classList.remove("is-copied", "is-error");
      }, 1600);
    });

    buttonContainer.appendChild(button);
  });
});
