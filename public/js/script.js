console.log('🚀 JavaScript loaded from C HTTP server!');

let clickCount = 0;
const colors = ['#f8fafc', '#fef3c7', '#dbeafe', '#fecaca', '#d1fae5', '#f3e8ff'];
let colorIndex = 0;

function initApp() {
    console.log('Initializing application...');
    setupHomePage();
    setupAboutPage();
    showSystemInfo();
}

function setupHomePage() {
    const testButton = document.getElementById('testButton');
    const message = document.getElementById('message');
    const countSpan = document.getElementById('count');
    
    if (testButton) {
        testButton.addEventListener('click', function() {
            clickCount++;
            
            if (countSpan) {
                countSpan.textContent = clickCount;
                animateElement(countSpan);
            }
            
            if (message) {
                message.classList.remove('hidden');
                message.classList.add('show');
                
                if (clickCount === 1) {
                    message.innerHTML = 'JavaScript working correctly! 🎉';
                } else if (clickCount === 5) {
                    message.innerHTML = 'Awesome! Your HTTP server handles JS perfectly 💪';
                } else if (clickCount === 10) {
                    message.innerHTML = '10 clicks! The server is very stable 🏆';
                } else if (clickCount > 10) {
                    message.innerHTML = `${clickCount} clicks! Your C server is incredible 🔥`;
                }
            }
            
            testButton.style.backgroundColor = getRandomColor();
            animateButton(testButton);
        });
    }
}

function setupAboutPage() {
    const colorButton = document.getElementById('colorButton');
    const info = document.getElementById('info');
    
    if (colorButton) {
        colorButton.addEventListener('click', function() {
            colorIndex = (colorIndex + 1) % colors.length;
            document.body.style.backgroundColor = colors[colorIndex];
            
            if (info) {
                info.innerHTML = `Background color changed: ${colors[colorIndex]} 🎨`;
                animateElement(info);
            }
            
            animateButton(colorButton);
            console.log(`Color changed to: ${colors[colorIndex]}`);
        });
    }
}

function showSystemInfo() {
    console.log('=== SYSTEM INFORMATION ===');
    console.log(`User Agent: ${navigator.userAgent}`);
    console.log(`Language: ${navigator.language}`);
    console.log(`Platform: ${navigator.platform}`);
    console.log(`Current URL: ${window.location.href}`);
    console.log(`Protocol: ${window.location.protocol}`);
    console.log(`Host: ${window.location.host}`);
    console.log('===========================');
    
    const systemInfo = document.getElementById('systemInfo');
    if (systemInfo) {
        systemInfo.innerHTML = `
            <strong>System Information:</strong><br>
            Browser: ${navigator.userAgent.split(' ')[0]}<br>
            Language: ${navigator.language}<br>
            URL: ${window.location.href}
        `;
    }
}

function getRandomColor() {
    const colors = ['#2563eb', '#059669', '#dc2626', '#7c3aed', '#ea580c', '#0891b2'];
    return colors[Math.floor(Math.random() * colors.length)];
}

function animateElement(element) {
    element.style.transform = 'scale(1.05)';
    element.style.transition = 'transform 0.2s ease';
    
    setTimeout(() => {
        element.style.transform = 'scale(1)';
    }, 200);
}

function animateButton(button) {
    button.style.transform = 'scale(0.95)';
    
    setTimeout(() => {
        button.style.transform = 'scale(1)';
    }, 150);
}

function testServerConnection() {
    console.log('Testing server connection...');
    const currentTime = new Date().toLocaleString();
    console.log(`Active connection at: ${currentTime}`);
    return true;
}

function onPageLoad() {
    console.log(`Page loaded: ${document.title}`);
    testServerConnection();
    setTimeout(() => {
        console.log('Your C HTTP server is working perfectly! 🎉');
    }, 1000);
}

document.addEventListener('DOMContentLoaded', function() {
    console.log('DOM fully loaded');
    initApp();
    onPageLoad();
});

window.addEventListener('load', function() {
    console.log('All resources loaded (HTML, CSS, JS, images)');
});

window.addEventListener('error', function(e) {
    console.error('JavaScript error:', e.error);
});

function showStats() {
    return {
        clicks: clickCount,
        currentColor: colors[colorIndex],
        pageTitle: document.title,
        loadTime: performance.now()
    };
}

window.serverStats = showStats;
window.testConnection = testServerConnection;
