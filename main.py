import asyncio
import logging
from aiogram import Bot, Dispatcher, types
from aiogram.filters import Command
from aiogram.methods import DeleteWebhook
from aiogram.types import Message
import requests

TOKEN = '7596093771:AAEuqTv9cUSuPNXUd_cGbfPsLN8GB-FTtvg'
AI_TOKEN = 'io-v2-eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJvd25lciI6ImNlNGU1YTJlLTljNTAtNDkyNy04OTY1LTY1OGJhNzI0MzcxZiIsImV4cCI6NDkwMjM1Nzc4N30.EDhzonH1WO5OhDMK_0GVJk6bn-LqK-ISoXetrD0cbtvhnoFSBNjiimBSMDpLUUSyNRumC6SPVsP8Ep8bDBueAg'

url = "https://api.intelligence.io.solutions/api/v1/chat/completions"
headers = {
    "Content-Type": "application/json",
    "Authorization": f"Bearer {AI_TOKEN}",
}

user_sessions = {}
logging.basicConfig(level=logging.INFO)
bot = Bot(TOKEN)
dp = Dispatcher()

@dp.message(Command("start"))
async def cmd_start(message: types.Message):
    user_sessions[message.from_user.id] = []
    await message.answer("👋 Привіт! Я перший ші бот якій уміє пиздити цепком задвай питання", parse_mode='HTML')

@dp.message(lambda message: message.text)
async def handle_text(message: Message):
    user_id = message.from_user.id
    user_history = user_sessions.get(user_id, [])

    user_history.append({"role": "user", "content": message.text})
    if len(user_history) > 10:
        user_history = user_history[-10:]

    data = {
        "model": "mistralai/Mistral-Large-Instruct-2411",
        "messages": [
            {"role": "system", "content": (
                "Відповідай як досвідчений програміст. Пиши якісний код, виправляй помилки, роби рев'ю. "
                "Завжди відповідай українською."
            )}
        ] + user_history,
    }

    try:
        response = requests.post(url, headers=headers, json=data)
        response.raise_for_status()
        response_json = response.json()

        try:
            text = response_json['choices'][0]['message']['content']
            bot_text = text.split('</think>\n\n')[-1]
        except (KeyError, IndexError):
            bot_text = "⚠️ Помилка: аааа піздец баклажани рвут резину невдалося прочітати текст."

        user_history.append({"role": "assistant", "content": bot_text})
        user_sessions[user_id] = user_history

        await message.answer(bot_text, parse_mode="Markdown")

    except requests.exceptions.RequestException as e:
        logging.error(f"API request failed: {e}")
        await message.answer("⚠️ Помилка у ші бачок потік невідповідає.", parse_mode="Markdown")

@dp.message(lambda message: message.photo)
async def handle_image(message: types.Message):
    try:
        file_id = message.photo[-1].file_id
        file = await bot.get_file(file_id)
        file_path = file.file_path
        file_url = f"https://api.telegram.org/file/bot{TOKEN}/{file_path}"

        data = {
            "model": "meta-llama/Llama-3.2-90B-Vision-Instruct",
            "messages": [
                {
                    "role": "system",
                    "content": "Ти AI-інженер. Проаналізуй зображення та опиши технічні деталі українською."
                },
                {
                    "role": "user",
                    "content": [
                        {"type": "text", "text": "Опиши це зображення."},
                        {"type": "image_url", "image_url": {"url": file_url}}
                    ]
                }
            ]
        }

        response = requests.post(url, headers=headers, json=data)
        response.raise_for_status()
        response_json = response.json()

        try:
            text = response_json['choices'][0]['message']['content']
        except (KeyError, IndexError):
            text = "⚠️ Помилка: піздец блятскому хутору не отвічяє."

        await message.answer(text, parse_mode="Markdown")

    except requests.exceptions.RequestException as e:
        logging.error(f"API request failed: {e}")
        await message.answer("⚠️ Помилка шото із ізображеніем ебать.", parse_mode="Markdown")
    except Exception as e:
        logging.error(f"Unexpected error: {e}")
        await message.answer("⚠️ Сталася хуй пойми яка помилка.", parse_mode="Markdown")

async def main():
    await bot(DeleteWebhook(drop_pending_updates=True))
    await dp.start_polling(bot)

if __name__ == "__main__":
    asyncio.run(main())
