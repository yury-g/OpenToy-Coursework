for (const button of document.querySelectorAll('[data-filter]')) {
  button.addEventListener('click', () => {
    const filter = button.dataset.filter;
    let count = 0;
    for (const project of document.querySelectorAll('[data-category]')) {
      project.hidden = filter !== 'all' && project.dataset.category !== filter;
      if (!project.hidden) count++;
    }
    for (const peer of document.querySelectorAll('[data-filter]')) peer.setAttribute('aria-pressed', String(peer === button));
    document.getElementById('result-count').textContent = `${count} project${count === 1 ? '' : 's'}`;
  });
}
